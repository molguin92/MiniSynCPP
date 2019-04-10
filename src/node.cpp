/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "node.h"
#include "net/protocol.h"
#include <string.h>
#include <unistd.h>
#include <protocol.pb.h>
#include <google/protobuf/message.h>

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

    if (bind(this->sock_fd, (struct sockaddr*) &this->local_addr, sizeof(this->local_addr)) < 0)
    {
        // TODO: Handle error
        exit(1);
    }
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

    uint64_t timestamp = this->current_time_ns(); // timestamp BEFORE passing on to network stack
    if (sendto(this->sock_fd, reply_buf, out_sz, 0, dest, sizeof(*dest)) != out_sz)
        // TODO: HANDLE ERROR
        exit(1);

    return timestamp;
}

uint64_t MiniSync::Node::recv_message(MiniSync::Protocol::MiniSyncMsg& msg, struct sockaddr* reply_to)
{
    uint8_t buf[MiniSync::Protocol::MAX_MSG_LEN];
    ssize_t recv_sz;
    socklen_t reply_to_len = sizeof(*reply_to);
    msg.Clear();

    memset(reply_to, 0x00, reply_to_len);
    // sizeof(uint8_t) SHOULD be 1, but it doesn't hurt to be explicit to avoid bugs
    memset(buf, 0x00, MiniSync::Protocol::MAX_MSG_LEN * sizeof(uint8_t));

    if ((recv_sz = recvfrom(this->sock_fd, buf, MiniSync::Protocol::MAX_MSG_LEN, 0, reply_to, &reply_to_len)) < 0)
        // TODO: HANDLE ERROR
        exit(1);

    uint64_t timestamp = this->current_time_ns(); // timestamp after receiving whole message

    // parse buffer into a protobuf message
    msg.ParseFromArray(buf, recv_sz); // TODO: handle error while parsing
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

    // TODO: finish
}

void MiniSync::SyncNode::sync()
{

}

MiniSync::SyncNode::SyncNode(uint16_t bind_port,
                             std::string& peer,
                             uint16_t peer_port,
                             const MiniSync::SyncAlgorithm& sync_algo) :
Node(bind_port, MiniSync::Protocol::NodeMode::SYNC),
algo(sync_algo),
peer(peer),
peer_port(peer_port),
peer_addr(SOCKADDR{})
{
    // set up peer addr
    memset(&this->peer_addr, 0, sizeof(SOCKADDR));

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(this->peer.c_str());
    peer_addr.sin_port = htons(this->peer_port);
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
        recv_time_ns = this->recv_message(incoming, nullptr);
        switch (incoming.payload_case())
        {
            case Protocol::MiniSyncMsg::kBeacon:
            {
                const MiniSync::Protocol::Beacon& beacon = incoming.beacon();
                reply.set_seq(beacon.seq());
                reply.set_beacon_recv_time(recv_time_ns);

                outgoing.set_allocated_beacon_r(&reply);
                reply.set_reply_send_time(Node::current_time_ns());

                this->send_message(outgoing, nullptr);

                // clean up after send
                outgoing.Clear();
                reply.Clear();
                break;
            }
            case Protocol::MiniSyncMsg::kGoodbye:
            {
                // got goodbye, reply and shutdown
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
        this->recv_message(incoming, &reply_to);
        switch (incoming.payload_case())
        {
            case Protocol::MiniSyncMsg::kHandshake:
            {
                using ReplyStatus = MiniSync::Protocol::HandshakeReply_Status;
                const auto& handshake = incoming.handshake();

                if (Protocol::VERSION_MAJOR != handshake.version_major() ||
                    Protocol::VERSION_MINOR != handshake.version_minor())
                    reply.set_status(ReplyStatus::HandshakeReply_Status_VERSION_MISMATCH);
                else if (handshake.mode() == this->mode)
                    reply.set_status(ReplyStatus::HandshakeReply_Status_MODE_MISMATCH);
                else
                {
                    // everything is ok, let's "connect"
                    reply.set_status(ReplyStatus::HandshakeReply_Status_SUCCESS);
                    // UDP is connectionless, this is merely to store the address of the client and "fake" a connection
                    connect(this->sock_fd, (struct sockaddr*) &reply_to, reply_to_len);
                    listening = false;
                }

                // reply is sent no matter what
                outgoing.set_allocated_handshake_r(&reply);
                this->send_message(outgoing, &reply_to);

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
