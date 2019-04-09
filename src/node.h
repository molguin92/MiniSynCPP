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

namespace MiniSync
{
    typedef struct sockaddr_in SOCKADDR;

    class Node
    {
    protected:
        enum MODE : uint8_t
        {
            SYNC = 0x00,
            REF = 0x01
        };

        int sock_fd;
        uint32_t bind_port;
        SOCKADDR local_addr;
        SOCKADDR peer_addr;
        const MODE mode;

        Node(uint16_t bind_port, MODE mode);
        ~Node();

        static uint64_t current_time_ns();

    public:
        virtual void run() = 0;
    };

    class ReferenceNode : Node
    {
    public:
        ReferenceNode(uint16_t bind_port) :
        Node(bind_port, MODE::REF)
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
        const MiniSync::SyncAlgorithm& algo;
        void handshake();
        void sync();

    public:
        SyncNode(uint16_t bind_port, std::string& peer, uint16_t peer_port, const MiniSync::SyncAlgorithm& sync_algo);
        ~SyncNode() = default;

        void run() final;
    };
}

#endif //MINISYNCPP_NODE_H
