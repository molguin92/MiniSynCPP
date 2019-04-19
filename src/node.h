/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef MINISYNCPP_NODE_H
#define MINISYNCPP_NODE_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include "algorithms/minisync.h"
#include <protocol.pb.h>
#include "stats.h"

namespace MiniSync
{
    typedef struct sockaddr_in SOCKADDR;

    class Node
    {
    protected:
        std::chrono::steady_clock::time_point start; // TODO: timestamp

        int sock_fd;
        uint16_t bind_port;
        SOCKADDR local_addr;
        const MiniSync::Protocol::NodeMode mode;
        std::atomic_bool running;

        Node(uint16_t bind_port, MiniSync::Protocol::NodeMode mode);

        us_t send_message(MiniSync::Protocol::MiniSyncMsg& msg, const sockaddr* dest);
        us_t recv_message(MiniSync::Protocol::MiniSyncMsg& msg, struct sockaddr* reply_to);

    public:
        virtual void run() = 0;
        virtual void shut_down();
        virtual ~Node();
    };

    class ReferenceNode : public Node
    {
    public:
        explicit ReferenceNode(uint16_t bind_port);
        ~ReferenceNode() override = default;

        void run() final;

    private:
        void serve();
        void wait_for_handshake();
    };

    class SyncNode : public Node
    {
    private:
        const std::string& peer;
        const uint16_t peer_port;
        SOCKADDR peer_addr;
        const std::string stat_file_path;
        std::unique_ptr<MiniSync::SyncAlgorithm> algo;
        MiniSync::Stats::SyncStats stats;
        void handshake();
        void sync();

    public:
        static const uint32_t RD_TIMEOUT_USEC = 100000; // 100 ms

        SyncNode(uint16_t bind_port,
                 std::string& peer,
                 uint16_t peer_port,
                 std::unique_ptr<MiniSync::SyncAlgorithm>&& sync_algo,
                 std::string stat_file_path = "");
        ~SyncNode() override; // = default;

        void run() final;
    };
}

#endif //MINISYNCPP_NODE_H
