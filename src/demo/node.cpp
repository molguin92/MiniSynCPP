/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <utility>
#include <cstring>
#include <unistd.h>
#include <protocol.pb.h>
#include <google/protobuf/message.h>
#include <thread>
#include <demo_config.h>
#include "node.h"
#include "exception.h"
#include "loguru/loguru.hpp"
//#include "algorithms/constraints.h"

#ifdef __x86_64__
#define PRISIZE_T PRIu64
#else
#define PRISIZE_T PRId32
#endif

MiniSync::Node::Node(uint16_t bind_port, MiniSync::Protocol::NodeMode mode) :
    bind_port(bind_port), local_addr(SOCKADDR{}), mode(mode), running(true)
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
    uint_fast32_t total_samples = 5000; // TODO: parametrize
    uint_fast32_t considered_samples = 1000;

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
    // set a global time reference point for using steady_clock timestamps
    auto T0 = std::chrono::steady_clock::now();
    // set up a separate thread for echoing messages
    std::thread t([loop_in_fd, total_samples, T0]()
                  {
                      uint8_t in_buf[MAX_MSG_LEN] = {0x00};
                      uint8_t out_buf[MAX_MSG_LEN] = {0x00};
                      sockaddr_in reply_to{0x00};
                      socklen_t reply_to_len = sizeof(sockaddr_in);
                      ssize_t in_msg_len;
                      ssize_t out_msg_len;

                      MiniSync::Protocol::MiniSyncMsg msg_bcn{};
                      MiniSync::Protocol::MiniSyncMsg reply{};
                      reply.set_allocated_beacon_r(new MiniSync::Protocol::BeaconReply{});

                      std::chrono::steady_clock::time_point t_in;

                      for (int_fast32_t i = 0; i < total_samples; ++i)
                      {
                          // read incoming
                          CHECK_GE_F((in_msg_len = recvfrom(loop_in_fd, in_buf, MAX_MSG_LEN, 0,
                                                            (sockaddr*) &reply_to, &reply_to_len)), 0,
                                     "Error reading from socket on loopback interface...");
                          t_in = std::chrono::steady_clock::now();

                          msg_bcn.ParseFromArray(in_buf, in_msg_len);
                          reply.mutable_beacon_r()->set_seq(msg_bcn.beacon().seq());
                          reply.mutable_beacon_r()->set_beacon_recv_time(
                              std::chrono::duration_cast<std::chrono::nanoseconds>(t_in - T0).count()
                          );
                          reply.mutable_beacon_r()->set_reply_send_time(
                              std::chrono::duration_cast<std::chrono::nanoseconds>(
                                  std::chrono::steady_clock::now() - T0).count()
                          );

                          // send it right back...
                          out_msg_len = reply.ByteSizeLong();
                          reply.SerializeToArray(out_buf, out_msg_len);
                          CHECK_GE_F(sendto(loop_in_fd, out_buf, out_msg_len, 0,
                                            (sockaddr*) &reply_to, reply_to_len), 0,
                                     "Error writing to socket on loopback interface.");

                          // cleanup
                          memset(in_buf, 0x00, MAX_MSG_LEN);
                          memset(out_buf, 0x00, MAX_MSG_LEN);
                          memset(&reply_to, 0x00, reply_to_len);
                      }
                  });

    // measure
    MiniSync::Protocol::MiniSyncMsg bcn_msg{};
    MiniSync::Protocol::MiniSyncMsg rpl_msg{};
    bcn_msg.set_allocated_beacon(new MiniSync::Protocol::Beacon{});
    uint8_t beacon_buf[MAX_MSG_LEN] = {0x00};
    uint8_t reply_buf[MAX_MSG_LEN] = {0x00};
    ssize_t out_sz, in_sz;

    us_t min_bcn_delay{std::numeric_limits<long double>::max()};
    us_t min_rpl_delay{std::numeric_limits<long double>::max()};
    std::chrono::steady_clock::duration t_out, t_in;
    socklen_t addr_sz = sizeof(sockaddr_in);
    for (int_fast32_t i = 0; i < total_samples; ++i)
    {
        bcn_msg.mutable_beacon()->set_seq(i % UINT8_MAX);
        out_sz = bcn_msg.ByteSizeLong();
        bcn_msg.SerializeToArray(beacon_buf, out_sz);
        t_out = std::chrono::steady_clock::now() - T0;
        // send payload
        CHECK_GE_F(sendto(loop_out_fd, beacon_buf, out_sz, 0, (sockaddr*) &loopback_addr, addr_sz), 0,
                   "Error writing to socket on loopback interface.");

        // get reply
        CHECK_GE_F((in_sz = recvfrom(loop_out_fd, reply_buf, MAX_MSG_LEN, 0, nullptr, nullptr)), 0,
                   "Error reading from socket on loopback interface...");
        t_in = std::chrono::steady_clock::now() - T0;
        rpl_msg.ParseFromArray(reply_buf, in_sz);

        if (i >= total_samples - considered_samples)
        {
            // only consider the last n samples, to give the processor some time to spin up
            // TODO: parametrize number of considered samples and total number of samples
            // delays include both the delay through the network stack on the way out and on the way in, so divide by 2.0
            // we assume symmetric delays
            auto bcn_delay = (std::chrono::nanoseconds{rpl_msg.beacon_r().beacon_recv_time()} - t_out) / 2.0;
            auto rpl_delay = (t_in - std::chrono::nanoseconds{rpl_msg.beacon_r().reply_send_time()}) / 2.0;
            min_bcn_delay = std::min(std::chrono::duration_cast<us_t>(bcn_delay), min_bcn_delay);
            min_rpl_delay = std::min(std::chrono::duration_cast<us_t>(rpl_delay), min_rpl_delay);
        }

        // cleanup
        memset(beacon_buf, 0x00, MAX_MSG_LEN);
        memset(reply_buf, 0x00, MAX_MSG_LEN);
    }
    t.join();

    this->minimum_delays.beacon = min_bcn_delay;
    this->minimum_delays.beacon_reply = min_rpl_delay;

    LOG_F(INFO, "Minimum latencies through the network stack:");
    LOG_F(INFO, "Beacons: %Lf µs", min_bcn_delay.count());
    LOG_F(INFO, "Beacon replies: %Lf µs", min_rpl_delay.count());
    shutdown(loop_in_fd, SHUT_RDWR);
    shutdown(loop_out_fd, SHUT_RDWR);
    close(loop_in_fd);
    close(loop_out_fd);
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
    uint8_t buf[MAX_MSG_LEN] = {0x00};
    ssize_t recv_sz;
    socklen_t reply_to_len = sizeof(struct sockaddr_in);
    msg.Clear();

    DLOG_F(INFO, "Listening for incoming messages...");
    if (reply_to != nullptr)
        memset(reply_to, 0x00, reply_to_len);

    if ((recv_sz = recvfrom(this->sock_fd, buf, MAX_MSG_LEN, 0, reply_to, &reply_to_len)) < 0)
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
    msg.set_allocated_handshake(new MiniSync::Protocol::Handshake{});
    msg.mutable_handshake()->set_mode(this->mode);
    msg.mutable_handshake()->set_version_major(PROTOCOL_VERSION_MAJOR);
    msg.mutable_handshake()->set_version_minor(PROTOCOL_VERSION_MINOR);

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
    ssize_t send_sz, recv_sz;

    us_t min_uplink_delay{0}, min_downlink_delay{0};

    while (this->running.load())
    {
        auto t_i = std::chrono::steady_clock::now();
        msg.set_allocated_beacon(new MiniSync::Protocol::Beacon{});
        msg.mutable_beacon()->set_seq(seq);
        send_sz = msg.ByteSizeLong();

        LOG_F(INFO, "Sending beacon (SEQ %"
            PRIu8
            ").", seq);

        MiniSync::Protocol::BeaconReply reply;

        try
        {
            // nullptr since we should already be connected
            to = this->send_message(msg, nullptr);

            for (;;)
            {
                // wait for reply without resending to avoid ugly feedback loops
                tr = this->recv_message(msg, nullptr);
                recv_sz = msg.ByteSizeLong();

                if (!msg.has_beacon_r())
                {
                    LOG_F(WARNING, "Got a message from peer which was not a beacon reply.");
                    continue;
                }
                else if ((reply = msg.beacon_r()).seq() != seq)
                {
                    LOG_F(WARNING, "Beacon reply was out of order, ignoring...");
                    continue;
                }
                else break; // reply is valid
            }
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


        // timestamps are in nanoseconds, but protocol works with microseconds
        tbr = us_t{std::chrono::nanoseconds{reply.beacon_recv_time()}};
        tbt = us_t{std::chrono::nanoseconds{reply.reply_send_time()}};

        // adjust local timestamps with minimum delays for beacons and replies
        to += this->minimum_delays.beacon;
        tr -= this->minimum_delays.beacon_reply;

        // additional adjustment based on bandwidth
        if (this->bw_bytes_per_usecond > 0)
        {
            // ACK size on WiFi is 14 bytes, so let's use that as the minimum frame size
            send_sz = std::max(send_sz, static_cast<ssize_t>(14));
            recv_sz = std::max(recv_sz, static_cast<ssize_t>(14));
            min_uplink_delay = us_t{send_sz / bw_bytes_per_usecond};
            min_downlink_delay = us_t{recv_sz / bw_bytes_per_usecond};
        }

        // adjustment based on ping
        min_uplink_delay = std::max(min_uplink_delay, this->min_ping_oneway_us);
        min_downlink_delay = std::max(min_downlink_delay, this->min_ping_oneway_us);

        to += min_uplink_delay;
        tr -= min_downlink_delay;

        // add data points
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
}

MiniSync::SyncNode::SyncNode(uint16_t bind_port,
                             std::string& peer,
                             uint16_t peer_port,
                             std::shared_ptr<MiniSync::API::Algorithm>&& sync_algo,
                             std::string stat_file_path,
                             double bandwidth_mbps,
                             double min_ping_rtt_ms) :
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

    // store bandwidth in bytes/µsecond
    // 1 s = 1 000 000 µs
    // 1 Megabit = 1 000 000 bytes / 8
    // -> X * 1 Megabit / s = X * (1 000 000 bytes / 8) / 1 000 000 µs
    // = X / 8.0 bytes/µs
    if (bandwidth_mbps > 0)
    {
        this->bw_bytes_per_usecond = bandwidth_mbps / 8.0;
        LOG_F(INFO, "User specified bandwidth: %f Mbps | %f bytes per µs", bandwidth_mbps, bw_bytes_per_usecond);
    }
    else this->bw_bytes_per_usecond = -1.0;

    // ping rtt
    // we multiply it by a factor of 0.9 since we want the absolute minimum with a bit of leeway as well
    this->min_ping_oneway_us = min_ping_rtt_ms > 0 ? us_t{min_ping_rtt_ms * 1000.0 / 2.0 * 0.9} : us_t{-1.0};
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
                const MiniSync::Protocol::Beacon& beacon = incoming.beacon();
                outgoing.set_allocated_beacon_r(new MiniSync::Protocol::BeaconReply{});
                outgoing.mutable_beacon_r()->set_seq(beacon.seq());
                outgoing.mutable_beacon_r()->set_beacon_recv_time(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        recv_time - this->minimum_delays.beacon).count()); // adjust with minimum delays

                LOG_F(INFO, "Received a beacon (SEQ %"
                    PRIu8
                    ").", beacon.seq());

                outgoing.mutable_beacon_r()->set_reply_send_time(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        (std::chrono::steady_clock::now() - start) + this->minimum_delays.beacon_reply).count());

                LOG_F(INFO, "Replying to beacon.");
                this->send_message(outgoing, nullptr);
            }
            else if (incoming.has_goodbye())
            {
                // got goodbye, reply and shutdown
                LOG_F(WARNING, "Got shutdown request.");
                outgoing.set_allocated_goodbye_r(new MiniSync::Protocol::GoodByeReply{});
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
                outgoing.set_allocated_handshake_r(new MiniSync::Protocol::HandshakeReply{});
                LOG_F(INFO, "Received handshake request.");
                using ReplyStatus = MiniSync::Protocol::HandshakeReply_Status;
                const auto& handshake = incoming.handshake();

                if (PROTOCOL_VERSION_MAJOR != handshake.version_major() ||
                    PROTOCOL_VERSION_MINOR != handshake.version_minor())
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
                          PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MINOR,
                          handshake.version_major(), handshake.version_minor());
                    outgoing.mutable_handshake_r()->set_status(ReplyStatus::HandshakeReply_Status_VERSION_MISMATCH);
                }
                else if (handshake.mode() == this->mode)
                {
                    LOG_F(WARNING, "Handshake: Mode mismatch.");
                    outgoing.mutable_handshake_r()->set_status(ReplyStatus::HandshakeReply_Status_MODE_MISMATCH);
                }
                else
                {
                    // everything is ok, let's "connect"
                    LOG_F(INFO, "Handshake successful.");
                    outgoing.mutable_handshake_r()->set_status(ReplyStatus::HandshakeReply_Status_SUCCESS);
                    // UDP is connectionless, this is merely to store the address of the client and "fake" a connection
                    CHECK_GE_F(connect(this->sock_fd, &reply_to, reply_to_len), 0,
                               "Call to connect failed. ERRNO: %s", strerror(errno));
                    this->start = std::chrono::steady_clock::now(); // start counting time
                    listening = false;
                }

                // reply is sent no matter what
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
