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
        int sock_fd;
        uint32_t bind_port;
        SOCKADDR local_addr;
        SOCKADDR peer_addr;
        std::string peer;
        uint32_t peer_port;

        Node(uint16_t bind_port, std::string& peer, uint16_t peer_port);
    };

    class ReferenceNode : Node
    {
    public:
        ReferenceNode(uint16_t bind_port, std::string& peer, uint16_t peer_port);
    };

    class SyncNode : Node
    {
    public:
        SyncNode(uint16_t bind_port, std::string& peer, uint16_t peer_port, MiniSync::SyncAlgorithm sync_algo);
    };
}

#endif //MINISYNCPP_NODE_H
