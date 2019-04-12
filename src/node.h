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
#include "minisync.h"
#include <protocol.pb.h>

namespace MiniSync
{
    typedef struct sockaddr_in SOCKADDR;

    class Node
    {
    protected:
        int sock_fd;
        uint32_t bind_port;
        SOCKADDR local_addr;
        const MiniSync::Protocol::NodeMode mode;

        Node(uint16_t bind_port, MiniSync::Protocol::NodeMode mode);
        ~Node();

        static uint64_t current_time_ns();
        uint64_t send_message(MiniSync::Protocol::MiniSyncMsg& msg, const sockaddr* dest);
        uint64_t recv_message(MiniSync::Protocol::MiniSyncMsg& msg, struct sockaddr* reply_to);

    public:
        virtual void run() = 0;
    };

    class ReferenceNode : Node
    {
    public:
        ReferenceNode(uint16_t bind_port) :
        Node(bind_port, MiniSync::Protocol::NodeMode::REFERENCE)
        {};
        ~ReferenceNode() = default;

        void run() final;

    private:
        void serve();
        void wait_for_handshake();
    };

    class SyncNode : Node
    {
    private:
        const std::string& peer;
        const uint16_t peer_port;
        SOCKADDR peer_addr;
        MiniSync::SyncAlgorithm& algo;
        void handshake();
        void sync();

    public:
        static const uint32_t RD_TIMEOUT_USEC = 100000; // 100 ms

        SyncNode(uint16_t bind_port, std::string& peer, uint16_t peer_port, MiniSync::SyncAlgorithm& sync_algo);

        ~SyncNode() = default;

        void run() final;
    };
}

#endif //MINISYNCPP_NODE_H
