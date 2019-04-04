/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "node.h"
#include <string.h>

MiniSync::Node::Node(uint16_t bind_port, std::string& peer, uint16_t peer_port) :
bind_port(bind_port), local_addr(SOCKADDR{}), peer_addr(SOCKADDR{})
{
    this->sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&this->local_addr, 0, sizeof(this->local_addr));
    this->local_addr.sin_family = AF_INET;
    this->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    this->local_addr.sin_port = htons(bind_port);

    if (bind(this->sock_fd, (struct sockaddr*) &this->local_addr, sizeof(this->local_addr)) < 0)
    {
        // TODO: Handle error
        exit(1);
    }

    this->peer = peer;
    this->peer_port = peer_port;
    memset(&this->peer_addr, 0, sizeof(this->peer_addr));
    this->peer_addr.sin_family = AF_INET;
    this->peer_addr.sin_addr.s_addr = inet_addr(peer.c_str());
    this->peer_addr.sin_port = htons(peer_port);
}

MiniSync::SyncNode::SyncNode(uint16_t bind_port,
                             std::string& peer,
                             uint16_t peer_port,
                             const MiniSync::SyncAlgorithm& sync_algo) :
Node(bind_port, peer, peer_port), algo(sync_algo)
{

}
