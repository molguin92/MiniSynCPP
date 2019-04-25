#include <utility>

/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "node.h"
#include "net/protocol.h"
#include "exception.h"
#include <cstring>
#include <unistd.h>
#include <protocol.pb.h>
#include <google/protobuf/message.h>
#include <thread>
#include "loguru/loguru.hpp"
#include "algorithms/constraints.h"

#ifdef __x86_64__
#define PRISIZE_T PRIu64
#else
#define PRISIZE_T PRId32
#endif

MiniSync::Node::Node(uint16_t bind_port, MiniSync::Protocol::NodeMode mode) :
    bind_port(bind_port), local_addr(SOCKADDR{}), mode(mode), running(true), minimum_delay(0)
{
    this->sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int enable = 1;
    setsockopt(this->sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    memset(&this->local_addr, 0, sizeof(this->local_addr));
    this->local_addr.sin_family = AF_INET;
    this->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this->local_addr.sin_port = htons(bind_port);

    LOG_F(INFO, "Binding UDP socket to port %"
        PRIu16
        "", this->bind_port);
    CHECK_GE_F(bind(this->sock_fd, (struct sockaddr*) &this->local_addr, sizeof(this->local_addr)), 0,
               "Failed to bind socket to UDP port %"
                   PRIu16, bind_port);

    // estimate minimum possible delay by looping messages on the loopback interface
    int loop_in_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int loop_out_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    uint16_t loopback_port = 5555; // TODO: Parametrize hardcoded port
    uint_fast32_t num_samples = 250; // TODO: parametrize

    setsockopt(loop_in_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    setsockopt(loop_out_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    sockaddr_in loopback_addr{0x00};
    memset(&loopback_addr, 0, sizeof(loopback_addr));
    loopback_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &loopback_addr.sin_addr);
    loopback_addr.sin_port = htons(loopback_port);

    CHECK_GE_F(bind(loop_in_fd, (struct sockaddr*) &loopback_addr, sizeof(loopback_addr)), 0,
               "Failed to bind UDP socket to loopback interface, port %d", loopback_port);

    // estimate
    // 1. set up payload
    srand(time(nullptr));
    MiniSync::Protocol::MiniSyncMsg payload;
    payload.set_allocated_beacon_r(new MiniSync::Protocol::BeaconReply{});
    payload.mutable_beacon_r()->set_seq(rand() % UINT8_MAX + 1);
    payload.mutable_beacon_r()->set_reply_send_time(std::chrono::nanoseconds{rand() % UINT64_MAX + 1}.count());
    payload.mutable_beacon_r()->set_beacon_recv_time(std::chrono::nanoseconds{rand() % UINT64_MAX + 1}.count());
    size_t payload_sz = payload.ByteSizeLong();
    uint8_t payload_b[payload_sz];
    payload.SerializeToArray(&payload_b, payload_sz);

    // 2. set up a separate thread for echoing messages
    std::thread t([loop_in_fd, loop_out_fd, payload_sz, num_samples]()
                  {
                      uint8_t incoming[payload_sz];
                      sockaddr_in reply_to{};
                      socklen_t reply_to_len = sizeof(sockaddr_in);
                      for (int_fast32_t i = 0; i < num_samples; ++i)
                      {
                          // read incoming
                          CHECK_GE_F(recvfrom(loop_in_fd, incoming, payload_sz, 0,
                                              (sockaddr*) &reply_to, &reply_to_len), 0,
                                     "Error reading from socket on loopback interface...");
                          // send it right back...
                          CHECK_GE_F(sendto(loop_in_fd, incoming, payload_sz, 0,
                                            (sockaddr*) &reply_to, reply_to_len), 0,
                                     "Error writing to socket on loopback interface.");
                      }
                  });

    // 3. measure
    // us_t samples[num_samples];
    us_t min_delay{std::numeric_limits<long double>::max()};
    us_t rtt;
    std::chrono::time_point<std::chrono::steady_clock> ti, tf;
    socklen_t addr_sz = sizeof(sockaddr_in);
    uint8_t incoming[payload_sz];
    sockaddr_in reply_to{};
    socklen_t reply_to_len = sizeof(sockaddr_in);
    for (int_fast32_t i = 0; i < num_samples; ++i)
    {
        ti = std::chrono::steady_clock::now();
        // send payload
        CHECK_GE_F(sendto(loop_out_fd, payload_b, payload_sz, 0,
                          (sockaddr*) &loopback_addr, addr_sz), 0,
                   "Error writing to socket on loopback interface.");
        // get it back
        CHECK_GE_F(recvfrom(loop_out_fd, incoming, payload_sz, 0,
                            (sockaddr*) &reply_to, &reply_to_len), 0,
                   "Error reading from socket on loopback interface...");
        tf = std::chrono::steady_clock::now();
        rtt = std::chrono::duration_cast<us_t>(tf - ti);

        // we only care about storing the absolute minimum possible delay
        // also, divide rtt by 2.0 since it includes delays for the message passing through the network stack twice
        min_delay = std::min(rtt / 2.0, min_delay);
    }
    t.join();

    LOG_F(INFO, "Minimum latency through the network stack: %Lf µs", min_delay.count());
    shutdown(loop_in_fd, SHUT_RDWR);
    shutdown(loop_out_fd, SHUT_RDWR);
    close(loop_in_fd);
    close(loop_out_fd);

    this->minimum_delay = min_delay;
}

MiniSync::Node::~Node()
{
    // close the socket on destruction
    LOG_F(WARNING, "Shutting down, bye bye!");
    if (this->running.load())
        this->shut_down();
    // shutdown(this->sock_fd, SHUT_RDWR);
    // close(this->sock_fd);
}

MiniSync::us_t
MiniSync::Node::send_message(MiniSync::Protocol::MiniSyncMsg& msg, const sockaddr* dest)
{
    size_t out_sz = msg.ByteSize();
    uint8_t reply_buf[out_sz];
    msg.SerializeToArray(reply_buf, out_sz);

    DLOG_F(INFO, "Sending a message of size %"
        PRISIZE_T
        " bytes...", out_sz);

    us_t timestamp = std::chrono::steady_clock::now() - start; // timestamp BEFORE passing on to network stack
    if (sendto(this->sock_fd, reply_buf, out_sz, 0, dest, sizeof(*dest)) != out_sz)
    {
        DLOG_F(WARNING, "Could not write to socket.");
        throw MiniSync::Exceptions::SocketWriteException();
    }

    DLOG_F(INFO, "Sent a message of size %"
        PRISIZE_T
        " bytes with timestamp %Lf µs...", out_sz, timestamp.count());
    return timestamp;
}

MiniSync::us_t
MiniSync::Node::recv_message(MiniSync::Protocol::MiniSyncMsg& msg, struct sockaddr* reply_to)
{
    uint8_t buf[MiniSync::Protocol::MAX_MSG_LEN] = {0x00};
    ssize_t recv_sz;
    socklen_t reply_to_len = sizeof(struct sockaddr_in);
    msg.Clear();

    DLOG_F(INFO, "Listening for incoming messages...");
    if (reply_to != nullptr)
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

    us_t timestamp = std::chrono::steady_clock::now() - start; // timestamp after receiving whole message

    DLOG_F(INFO, "Got %"
        PRISIZE_T
        " bytes of data at time %Lf µs.", recv_sz, timestamp.count());
    // deserialize buffer into a protobuf message
    if (!msg.ParseFromArray(buf, recv_sz))
    {
        DLOG_F(WARNING, "Failed to deserialize payload.");
        throw MiniSync::Exceptions::DeserializeMsgException();
    }
    return timestamp;
}

void MiniSync::Node::shut_down()
{
    // force clean shut down
    this->running.store(false);
    shutdown(this->sock_fd, SHUT_RDWR);
    close(this->sock_fd);
}

void MiniSync::SyncNode::run()
{
    this->handshake();
    this->sync();
}

/*
 * Simply listens for incoming beacons and replies accordingly.
 */
void MiniSync::ReferenceNode::run()
{
    this->wait_for_handshake();
    this->serve();
}

void MiniSync::SyncNode::handshake()
{
    // send handshake request to peer
    MiniSync::Protocol::MiniSyncMsg msg{};
    auto* handshake = new MiniSync::Protocol::Handshake{};

    handshake->set_mode(this->mode);
    handshake->set_version_major(MiniSync::Protocol::VERSION_MAJOR);
    handshake->set_version_minor(MiniSync::Protocol::VERSION_MINOR);

    msg.set_allocated_handshake(handshake);

    MiniSync::Protocol::MiniSyncMsg incoming{};
    while (this->running.load())
    {
        try
        {
            LOG_F(INFO, "Initializing handshake with peer %s:%"
                PRIu16
                ".", this->peer.c_str(), this->peer_port);
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
                               "Failed connecting socket to peer %s:%"
                                   PRIu16, this->peer.c_str(), this->peer_port);
                    // "start" local clock
                    this->start = std::chrono::steady_clock::now();
                    return;
                }

                case Protocol::HandshakeReply_Status_VERSION_MISMATCH:
                    LOG_F(ERROR, "Handshake failed: version mismatch.");
                case Protocol::HandshakeReply_Status_MODE_MISMATCH:
                    LOG_F(ERROR, "Handshake failed: Mode mismatch between the nodes.");
                case Protocol::HandshakeReply_Status_HandshakeReply_Status_INT_MIN_SENTINEL_DO_NOT_USE_:
                case Protocol::HandshakeReply_Status_HandshakeReply_Status_INT_MAX_SENTINEL_DO_NOT_USE_:
                case Protocol::HandshakeReply_Status_ERROR:
                    LOG_F(ERROR, "Handshake failed with unspecified error!");
                    this->running.store(false);
            }
        }
            // only catch expected exceptions, the rest we leave to the main code
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
    }
}

void MiniSync::SyncNode::sync()
{
    // send sync beacons and wait for timestamps
    MiniSync::Protocol::MiniSyncMsg msg{};

    us_t to, tbr, tbt, tr;
    uint8_t seq = 0;

    while (this->running.load())
    {
        auto t_i = std::chrono::steady_clock::now();
        auto* beacon = new MiniSync::Protocol::Beacon{};
        beacon->set_seq(seq);
        msg.set_allocated_beacon(beacon);

        LOG_F(INFO, "Sending beacon (SEQ %"
            PRIu8
            ").", seq);

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
            if (reply.seq() != seq)
            {
                // TODO: DO SOMETHING ABOUT OUT OF ORDER REPLIES
                LOG_F(WARNING, "Beacon reply was out of order, ignoring...");
                continue;
            }

            // timestamps are in nanoseconds, but protocol works with microseconds
            tbr = us_t{std::chrono::nanoseconds{reply.beacon_recv_time()}};
            tbt = us_t{std::chrono::nanoseconds{reply.reply_send_time()}};

            //LOG_F(INFO, "Timestamps:\nto=\t%Lf\ntbr=\t%Lf\ntbt=\t%Lf\ntr=\t%Lf",
            //      to.count(), tbr.count(), tbt.count(), tr.count());

            this->algo->addDataPoint(to, tbr, tr);
            this->algo->addDataPoint(to, tbt, tr);

            auto drift = algo->getDrift();
            auto drift_error = algo->getDriftError();
            auto offset = algo->getOffset();
            auto offset_error = algo->getOffsetError();

            auto
                adj_timestamp =
                std::chrono::duration_cast<us_t>(algo->getCurrentAdjustedTime().time_since_epoch());

            LOG_F(INFO, "Current adjusted timestamp: %Lf µs", adj_timestamp.count());
            LOG_F(INFO, "Drift: %Lf | Error: +/- %Lf", drift, drift_error);
            LOG_F(INFO, "Offset: %Lf µs | Error: +/- %Lf µs", offset.count(), offset_error.count());

            stats.add_sample(offset.count(), offset_error.count(), drift, drift_error);

            seq++;
            msg.Clear();
            // msg handles clearing beacon
            auto t_f = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(100) - (t_f - t_i)); // TODO: parameterize
        }
            // only catch exceptions that we can work with
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
    }
}

MiniSync::SyncNode::SyncNode(uint16_t bind_port,
                             std::string& peer,
                             uint16_t peer_port,
                             std::unique_ptr<MiniSync::SyncAlgorithm>&& sync_algo,
                             std::string stat_file_path) :
    Node(bind_port, MiniSync::Protocol::NodeMode::SYNC),
    algo(std::move(sync_algo)), // take ownership of algorithm
    peer(peer),
    peer_port(peer_port),
    peer_addr({}),
    stats({}),
    stat_file_path(std::move(stat_file_path))
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

MiniSync::SyncNode::~SyncNode()
{
    if (!this->stat_file_path.empty())
    {
        this->stats.write_csv(this->stat_file_path);
    }
}

void MiniSync::ReferenceNode::serve()
{
    // set up variables
    us_t recv_time;

    MiniSync::Protocol::MiniSyncMsg incoming{};
    MiniSync::Protocol::MiniSyncMsg outgoing{};

    // wait for beacons
    while (this->running.load())
    {
        // timestamp reception
        LOG_F(INFO, "Listening for incoming beacons.");
        try
        {
            recv_time = this->recv_message(incoming, nullptr);

            if (incoming.has_beacon())
            {
                // got beacon, so just reply
                auto* reply = new MiniSync::Protocol::BeaconReply{};
                const MiniSync::Protocol::Beacon& beacon = incoming.beacon();
                reply->set_seq(beacon.seq());
                reply->set_beacon_recv_time(std::chrono::duration_cast<std::chrono::nanoseconds>(recv_time).count());

                LOG_F(INFO, "Received a beacon (SEQ %"
                    PRIu8
                    ").", beacon.seq());

                outgoing.set_allocated_beacon_r(reply);
                reply->set_reply_send_time(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now() - start).count()
                );

                LOG_F(INFO, "Replying to beacon.");
                this->send_message(outgoing, nullptr);
            }
            else if (incoming.has_goodbye())
            {
                // got goodbye, reply and shutdown
                LOG_F(WARNING, "Got shutdown request.");
                auto* greply = new MiniSync::Protocol::GoodByeReply{};
                outgoing.set_allocated_goodbye_r(greply);
                this->send_message(outgoing, nullptr);
                this->running.store(false);
            }

            // clean up after send
            outgoing.Clear();
            // no need to clear up reply, outgoing takes care of it
        }
        catch (MiniSync::Exceptions::DeserializeMsgException& e)
        {
            // could not parse incoming message, just retry
            LOG_F(WARNING, "Could not deserialize incoming message, retrying...");
            continue;
        }
    }
}

void MiniSync::ReferenceNode::wait_for_handshake()
{
    // set up variables
    struct sockaddr reply_to{};
    socklen_t reply_to_len = sizeof(reply_to);

    MiniSync::Protocol::MiniSyncMsg incoming{};
    MiniSync::Protocol::MiniSyncMsg outgoing{};

    // discard anything that is not a handshake request
    bool listening = true;
    while (listening && this->running.load())
    {
        // wait for handshake
        LOG_F(INFO, "Waiting for incoming handshake requests.");
        try
        {
            this->recv_message(incoming, &reply_to);
            if (incoming.has_handshake())
            {
                auto* reply = new MiniSync::Protocol::HandshakeReply{};
                LOG_F(INFO, "Received handshake request.");
                using ReplyStatus = MiniSync::Protocol::HandshakeReply_Status;
                const auto& handshake = incoming.handshake();

                if (Protocol::VERSION_MAJOR != handshake.version_major() ||
                    Protocol::VERSION_MINOR != handshake.version_minor())
                {
                    LOG_F(WARNING, "Handshake: Version mismatch.");
                    LOG_F(WARNING, "Local version: %"
                        PRIu8
                        ".%"
                        PRIu8
                        " - Remote version: %"
                        PRIu8
                        ".%"
                        PRIu8,
                          Protocol::VERSION_MAJOR, Protocol::VERSION_MINOR,
                          handshake.version_major(), handshake.version_minor());
                    reply->set_status(ReplyStatus::HandshakeReply_Status_VERSION_MISMATCH);
                }
                else if (handshake.mode() == this->mode)
                {
                    LOG_F(WARNING, "Handshake: Mode mismatch.");
                    reply->set_status(ReplyStatus::HandshakeReply_Status_MODE_MISMATCH);
                }
                else
                {
                    // everything is ok, let's "connect"
                    LOG_F(INFO, "Handshake successful.");
                    reply->set_status(ReplyStatus::HandshakeReply_Status_SUCCESS);
                    // UDP is connectionless, this is merely to store the address of the client and "fake" a connection
                    CHECK_GE_F(connect(this->sock_fd, &reply_to, reply_to_len), 0,
                               "Call to connect failed. ERRNO: %s", strerror(errno));
                    this->start = std::chrono::steady_clock::now(); // start counting time
                    listening = false;
                }

                // reply is sent no matter what
                outgoing.set_allocated_handshake_r(reply);
                this->send_message(outgoing, &reply_to);
                // TODO: verify the handshake is received??

                // cleanup
                outgoing.Clear();
                // no need to clear reply, outgoing has ownership and will clear it
            }
        }
        catch (MiniSync::Exceptions::DeserializeMsgException& e)
        {
            // could not parse incoming message, just retry
            LOG_F(WARNING, "Could not deserialize incoming message, retrying...");
            continue;
        }
    }
}

MiniSync::ReferenceNode::ReferenceNode(uint16_t bind_port) :
    Node(bind_port, MiniSync::Protocol::NodeMode::REFERENCE)
{
    LOG_F(INFO, "Initializing ReferenceNode.");
}
