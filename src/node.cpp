/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "node.h"
#include "net/protocol.h"
#include "exception.h"
#include <string.h>
#include <unistd.h>
#include <protocol.pb.h>
#include <google/protobuf/message.h>
#include <thread>
#include "loguru/loguru.hpp"

MiniSync::Node::Node(uint16_t bind_port, MiniSync::Protocol::NodeMode mode) :
bind_port(bind_port), local_addr(SOCKADDR{}), mode(mode)
{
    this->sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int enable = 1;
    setsockopt(this->sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    memset(&this->local_addr, 0, sizeof(this->local_addr));
    this->local_addr.sin_family = AF_INET;
    this->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this->local_addr.sin_port = htons(bind_port);

    LOG_F(INFO, "Binding UDP socket to port %d", this->bind_port);
    CHECK_GE_F(bind(this->sock_fd, (struct sockaddr*) &this->local_addr, sizeof(this->local_addr)), 0,
               "Failed to bind socket to UDP port %d", bind_port);
}

uint64_t MiniSync::Node::current_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

MiniSync::Node::~Node()
{
    // close the socket on destruction
    shutdown(this->sock_fd, SHUT_RDWR);
    close(this->sock_fd);
}

uint64_t MiniSync::Node::send_message(MiniSync::Protocol::MiniSyncMsg& msg, const sockaddr* dest)
{
    size_t out_sz = msg.ByteSize();
    uint8_t reply_buf[out_sz];
    msg.SerializeToArray(reply_buf, out_sz);

    DLOG_F(INFO, "Sending a message of size %ld bytes...", out_sz);

    uint64_t timestamp = this->current_time_ns(); // timestamp BEFORE passing on to network stack
    if (sendto(this->sock_fd, reply_buf, out_sz, 0, dest, sizeof(*dest)) != out_sz)
    {
        DLOG_F(WARNING, "Could not write to socket.");
        throw MiniSync::Exceptions::SocketWriteException();
    }

    DLOG_F(INFO, "Sent a message of size %ld bytes with timestamp %ld...", out_sz, timestamp);
    return timestamp;
}

uint64_t MiniSync::Node::recv_message(MiniSync::Protocol::MiniSyncMsg& msg, struct sockaddr* reply_to)
{
    uint8_t buf[MiniSync::Protocol::MAX_MSG_LEN] = {0x00};
    ssize_t recv_sz;
    socklen_t reply_to_len = sizeof(*reply_to);
    msg.Clear();

    DLOG_F(INFO, "Listening for incoming messages...");
    memset(reply_to, 0x00, reply_to_len);
    if ((recv_sz = recvfrom(this->sock_fd, buf, MiniSync::Protocol::MAX_MSG_LEN, 0, reply_to, &reply_to_len)) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            DLOG_F(WARNING, "Timed out waiting for messages.");
            throw MiniSync::Exceptions::TimeoutException();
        }
        else throw MiniSync::Exceptions::SocketReadException();
    }

    uint64_t timestamp = this->current_time_ns(); // timestamp after receiving whole message

    DLOG_F(INFO, "Got a datagram at time %ld.", timestamp);
    // deserialize buffer into a protobuf message
    if (!msg.ParseFromArray(buf, recv_sz))
    {
        DLOG_F(WARNING, "Failed to deserialize payload.");
        throw MiniSync::Exceptions::DeserializeMsgException();
    }
    return timestamp;
}

void MiniSync::SyncNode::run()
{
    this->handshake();
    this->sync();
}

void MiniSync::SyncNode::handshake()
{
    // send handshake request to peer
    MiniSync::Protocol::MiniSyncMsg msg{};
    MiniSync::Protocol::Handshake handshake{};

    handshake.set_mode(this->mode);
    handshake.set_version_major(MiniSync::Protocol::VERSION_MAJOR);
    handshake.set_version_minor(MiniSync::Protocol::VERSION_MINOR);

    msg.set_allocated_handshake(&handshake);

    MiniSync::Protocol::MiniSyncMsg incoming{};
    bool success = false;
    while (!success)
    {
        try
        {
            LOG_F(INFO, "Initializing handshake with peer %s:%d.", this->peer.c_str(), this->peer_port);
            this->send_message(msg, (struct sockaddr*) &this->peer_addr);
            // wait for handshake response
            this->recv_message(incoming, (struct sockaddr*) &this->peer_addr);

            if (!incoming.has_handshake_r())
            {
                // message needs to be a handshake reply
                LOG_F(WARNING, "Got a message from peer which was not a handshake reply.");
                continue;
            }
            const MiniSync::Protocol::HandshakeReply& reply = incoming.handshake_r();
            switch (reply.status())
            {
                case Protocol::HandshakeReply_Status_SUCCESS:
                {
                    // if success, we can "connect" the socket and move on to actually synchronizing.
                    CHECK_GE_F(connect(this->sock_fd, (struct sockaddr*) &this->peer_addr, sizeof(this->peer_addr)), 0,
                               "Failed connecting socket to peer %s:%d", this->peer.c_str(), this->peer_port);
                    success = true;
                    break;
                }

                case Protocol::HandshakeReply_Status_VERSION_MISMATCH:
                    ABORT_F("Handshake failed: version mismatch.");
                case Protocol::HandshakeReply_Status_MODE_MISMATCH:
                    ABORT_F("Handshake failed: Mode mismatch between the nodes.");
                case Protocol::HandshakeReply_Status_HandshakeReply_Status_INT_MIN_SENTINEL_DO_NOT_USE_:
                case Protocol::HandshakeReply_Status_HandshakeReply_Status_INT_MAX_SENTINEL_DO_NOT_USE_:
                case Protocol::HandshakeReply_Status_ERROR:
                    ABORT_F("Handshake failed with unspecified error!");
            }
        }
        catch (MiniSync::Exceptions::SocketWriteException& e)
        {
            ABORT_F("%s", e.what());
        }
        catch (MiniSync::Exceptions::SocketReadException& e)
        {
            ABORT_F("%s", e.what());
        }
        catch (MiniSync::Exceptions::TimeoutException& e)
        {
            // if timed out, repeat!
            LOG_F(INFO, "Timed out waiting for handshake reply, retrying...");
            continue;
        }
        catch (MiniSync::Exceptions::DeserializeMsgException& e)
        {
            // Error while receiving message, probably malformed so just discard it
            LOG_F(INFO, "Failed to deserialize incoming message, retrying...");
            continue;
        }
        catch (MiniSync::Exceptions::SerializeMsgException& e)
        {
            ABORT_F("%s", e.what());
        }
    }
}

void MiniSync::SyncNode::sync()
{
    // send sync beacons and wait for timestamps
    MiniSync::Protocol::Beacon beacon{};
    MiniSync::Protocol::MiniSyncMsg msg{};
    bool running = true;

    uint64_t to, tbr, tbt, tr;
    uint8_t seq = 0;

    while (running)
    {
        beacon.set_seq(seq);
        msg.set_allocated_beacon(&beacon);

        LOG_F(INFO, "Sending beacon (SEQ %d).", seq);

        try
        {
            // nullptr since we should already be connected
            to = this->send_message(msg, nullptr);

            // wait for reply
            tr = this->recv_message(msg, nullptr);

            if (!msg.has_beacon_r())
            {
                LOG_F(WARNING, "Got a message from peer which was not a beacon reply.");
                continue;
            }

            const MiniSync::Protocol::BeaconReply& reply = msg.beacon_r();

            // ignore out-of-order replies
            if (beacon.seq() != seq)
            {
                LOG_F(WARNING, "Beacon reply was out of order, ignoring...");
                continue;
            }

            tbr = reply.beacon_recv_time();
            tbt = reply.reply_send_time();

            LOG_F(INFO, "Timestamps:\nto=\t%ld\ntbr=\t%ld\ntbt=\t%ld\ntr=\t%ld", to, tbr, tbt, tr);

            this->algo.addDataPoint(to, tbr, tr);
            this->algo.addDataPoint(to, tbt, tr);

            LOG_F(INFO, "Current adjusted timestamp: %ld", algo.getCurrentTimeNanoSeconds());
            LOG_F(INFO, "Drift: %f | Offset: %ld", algo.getDrift(), algo.getOffsetNanoSeconds());
            seq++;
            msg.Clear();
            beacon.Clear();

            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // TODO: parameterize
        }
        catch (MiniSync::Exceptions::TimeoutException& e)
        {
            // timed out, retry sending beacon
            LOG_F(INFO, "Timed out waiting for beacon reply, retrying...");
            continue;
        }
        catch (MiniSync::Exceptions::DeserializeMsgException& e)
        {
            // could not parse incoming message, just retry
            LOG_F(WARNING, "Could not deserialize incoming message, retrying...");
            continue;
        }
        catch (std::exception& e)
        {
            ABORT_F("%s", e.what());
        }

        beacon.Clear();
    }
}

MiniSync::SyncNode::SyncNode(uint16_t bind_port,
                             std::string& peer,
                             uint16_t peer_port,
                             MiniSync::SyncAlgorithm& sync_algo) :
Node(bind_port, MiniSync::Protocol::NodeMode::SYNC),
algo(sync_algo),
peer(peer),
peer_port(peer_port),
peer_addr(SOCKADDR{})
{
    LOG_F(INFO, "Initializing SyncNode.");
    // set up peer addr
    memset(&this->peer_addr, 0, sizeof(SOCKADDR));

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(this->peer.c_str());
    peer_addr.sin_port = htons(this->peer_port);

    // set a timeout for read operations, in order to send repeat beacons on timeouts
    struct timeval read_timeout{0x00};
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = MiniSync::SyncNode::RD_TIMEOUT_USEC;
    CHECK_EQ_F(setsockopt(this->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)), 0,
               "Failed setting SO_RCVTIMEO option for socket.");
}

/*
 * Simply listens for incoming beacons and replies accordingly.
 */
void MiniSync::ReferenceNode::run()
{
    this->wait_for_handshake();
    this->serve();
}

void MiniSync::ReferenceNode::serve()
{
    // set up variables
    uint64_t recv_time_ns;

    MiniSync::Protocol::MiniSyncMsg incoming{};
    MiniSync::Protocol::MiniSyncMsg outgoing{};
    MiniSync::Protocol::BeaconReply reply{};

    // wait for beacons
    bool listening = true;
    while (listening)
    {
        // timestamp reception
        LOG_F(INFO, "Listening for incoming beacons.");
        try
        {
            recv_time_ns = this->recv_message(incoming, nullptr);
        }
        catch (MiniSync::Exceptions::DeserializeMsgException& e)
        {
            // could not parse incoming message, just retry
            LOG_F(WARNING, "Could not deserialize incoming message, retrying...");
            continue;
        }
        catch (std::exception& e)
        {
            // TODO: More finegrained handling.
            ABORT_F("%s", e.what());
        }

        // TODO: change switch to if()?
        switch (incoming.payload_case())
        {
            case Protocol::MiniSyncMsg::kBeacon:
            {
                const MiniSync::Protocol::Beacon& beacon = incoming.beacon();
                reply.set_seq(beacon.seq());
                reply.set_beacon_recv_time(recv_time_ns);

                LOG_F(INFO, "Received a beacon (SEQ %u).", beacon.seq());

                outgoing.set_allocated_beacon_r(&reply);
                reply.set_reply_send_time(Node::current_time_ns());

                try
                {
                    LOG_F(INFO, "Replying to beacon.");
                    this->send_message(outgoing, nullptr);
                }
                catch (std::exception& e)
                {
                    // TODO: More finegrained handling.
                    ABORT_F("%s", e.what());
                }

                // clean up after send
                outgoing.Clear();
                reply.Clear();
                break;
            }
            case Protocol::MiniSyncMsg::kGoodbye:
            {
                // got goodbye, reply and shutdown
                LOG_F(WARNING, "Got shutdown request.");
                MiniSync::Protocol::GoodByeReply greply{};
                outgoing.set_allocated_goodbye_r(&greply);
                this->send_message(outgoing, nullptr);
                listening = false;
                break;
            }
            case Protocol::MiniSyncMsg::kHandshake:
            case Protocol::MiniSyncMsg::kHandshakeR:
            case Protocol::MiniSyncMsg::kBeaconR:
            case Protocol::MiniSyncMsg::kGoodbyeR:
            case Protocol::MiniSyncMsg::PAYLOAD_NOT_SET:
                break;
        }
    }
}

void MiniSync::ReferenceNode::wait_for_handshake()
{
    // set up variables
    sockaddr reply_to{};
    socklen_t reply_to_len = 0;

    MiniSync::Protocol::MiniSyncMsg incoming{};
    MiniSync::Protocol::MiniSyncMsg outgoing{};
    MiniSync::Protocol::HandshakeReply reply{};

    // discard anything that is not a handshake request
    bool listening = true;
    while (listening)
    {
        // wait for handshake
        LOG_F(INFO, "Waiting for incoming handshake requests.");
        try
        {
            this->recv_message(incoming, &reply_to);
        }
        catch (MiniSync::Exceptions::DeserializeMsgException& e)
        {
            // could not parse incoming message, just retry
            LOG_F(WARNING, "Could not deserialize incoming message, retrying...");
            continue;
        }
        catch (std::exception& e)
        {
            // TODO: finegrained handling
            ABORT_F("%s", e.what());
        }
        // TODO: condense switch into if()
        switch (incoming.payload_case())
        {
            case Protocol::MiniSyncMsg::kHandshake:
            {
                LOG_F(INFO, "Received handshake request.");
                using ReplyStatus = MiniSync::Protocol::HandshakeReply_Status;
                const auto& handshake = incoming.handshake();

                if (Protocol::VERSION_MAJOR != handshake.version_major() ||
                    Protocol::VERSION_MINOR != handshake.version_minor())
                {
                    LOG_F(WARNING, "Handshake: Version mismatch.");
                    LOG_F(WARNING, "Local version: %d.%d - Remote version: %d.%d",
                          Protocol::VERSION_MAJOR, Protocol::VERSION_MINOR,
                          handshake.version_major(), handshake.version_minor());
                    reply.set_status(ReplyStatus::HandshakeReply_Status_VERSION_MISMATCH);
                }
                else if (handshake.mode() == this->mode)
                {
                    LOG_F(WARNING, "Handshake: Mode mismatch.");
                    reply.set_status(ReplyStatus::HandshakeReply_Status_MODE_MISMATCH);
                }
                else
                {
                    // everything is ok, let's "connect"
                    LOG_F(INFO, "Handshake successful.");
                    reply.set_status(ReplyStatus::HandshakeReply_Status_SUCCESS);
                    // UDP is connectionless, this is merely to store the address of the client and "fake" a connection
                    connect(this->sock_fd, (struct sockaddr*) &reply_to, reply_to_len);
                    listening = false;
                }

                // reply is sent no matter what
                outgoing.set_allocated_handshake_r(&reply);
                try
                {
                    this->send_message(outgoing, &reply_to);
                    // TODO: verify the handshake is received??
                }
                catch (std::exception& e)
                {
                    // TODO: More finegrained handling.
                    ABORT_F("%s", e.what());
                }

                // cleanup
                outgoing.Clear();
                reply.Clear();
                break;
            }
                // ignore all other messages
            case Protocol::MiniSyncMsg::kHandshakeR:
            case Protocol::MiniSyncMsg::kBeacon:
            case Protocol::MiniSyncMsg::kBeaconR:
            case Protocol::MiniSyncMsg::kGoodbye:
            case Protocol::MiniSyncMsg::kGoodbyeR:
            case Protocol::MiniSyncMsg::PAYLOAD_NOT_SET:
                break;
        }
    }
}

MiniSync::ReferenceNode::ReferenceNode(uint16_t bind_port) :
Node(bind_port, MiniSync::Protocol::NodeMode::REFERENCE)
{
    LOG_F(INFO, "Initializing ReferenceNode.");
}
