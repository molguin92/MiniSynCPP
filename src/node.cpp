/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "node.h"
#include "net/protocol.h"
#include <string.h>

MiniSync::Node::Node(uint16_t bind_port, std::string& peer, uint16_t peer_port, MODE mode) :
bind_port(bind_port), local_addr(SOCKADDR{}), peer_addr(SOCKADDR{}), mode(mode)
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

uint64_t MiniSync::Node::current_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

MiniSync::SyncNode::SyncNode(uint16_t bind_port,
                             std::string& peer,
                             uint16_t peer_port,
                             const MiniSync::SyncAlgorithm& sync_algo) :
Node(bind_port, peer, peer_port, MODE::SYNC), algo(sync_algo)
{

}

/*
 * Simply listens for incoming beacons and replies accordingly.
 */
void MiniSync::ReferenceNode::run()
{
    // set up variables
    SOCKADDR reply_to{};
    socklen_t reply_to_len = 0;
    ssize_t recv_sz = -1;
    ssize_t sent_sz = -1;
    uint8_t buf[MiniSync::Protocol::MAX_MSG_LEN];
    uint64_t recv_time_ns;

    bool listening = true;

    // first, wait for handshake
    // prepare variables for incoming message
    memset(&reply_to, 0x00, sizeof(reply_to));
    memset(buf, 0x00, MiniSync::Protocol::MAX_MSG_LEN);
    recv_sz = -1;
    sent_sz = -1;
    reply_to_len = sizeof(reply_to);

    if ((recv_sz = recvfrom(this->sock_fd, buf, MiniSync::Protocol::MAX_MSG_LEN, 0,
                            (struct sockaddr*) &reply_to, &reply_to_len)) < 0)
        // TODO: HANDLE ERROR
        exit(1);

    // discard anything that is not a handshake request
    // TODO: session handling?
    while (listening)
    {
        switch (static_cast<Protocol::MSG_T>(buf[0]))
        {
            case Protocol::HANDSHAKE:
            {
                auto* handshake = (Protocol::MSG_HANDSHAKE*) &buf[1];
                Protocol::MSG_HANDSHAKE_REPLY reply{};
                memset(&reply, 0x00, sizeof(Protocol::MSG_HANDSHAKE_REPLY));

                if (Protocol::VERSION_MAJOR != handshake->prot_v_major ||
                    Protocol::VERSION_MINOR != handshake->prot_v_minor)
                    reply.status = Protocol::HANDSHAKE_STATUS::VERSION_MISMATCH;
                else if (static_cast<MODE>(handshake->node_mode) == this->mode)
                    reply.status = Protocol::HANDSHAKE_STATUS::MODE_MISMATCH;
                else
                    reply.status = Protocol::HANDSHAKE_STATUS::SUCCESS;

                size_t reply_len = sizeof(reply);
                size_t total_len = reply_len + sizeof(Protocol::MSG_T);
                uint8_t reply_buf[total_len];

                reply_buf[0] = Protocol::MSG_T::HANDSHAKE_REPLY;
                memcpy(reinterpret_cast<void*>(reply_buf[1]), &reply, reply_len);

                if ((sent_sz = sendto(this->sock_fd, reply_buf, total_len, 0, (struct sockaddr*) &reply_to,
                                      reply_to_len)) != total_len)
                    // TODO: HANDLE ERROR
                    exit(1);
                listening = false;
                break;
            }
            default:
                break;
        }
    }

    // now, we wait for beacons
    listening = true;
    while (listening)
    {
        // prepare variables for incoming message
        memset(&reply_to, 0x00, sizeof(reply_to));
        memset(buf, 0x00, MiniSync::Protocol::MAX_MSG_LEN);
        recv_sz = -1;
        sent_sz = -1;
        reply_to_len = sizeof(reply_to);

        if ((recv_sz = recvfrom(this->sock_fd, buf, MiniSync::Protocol::MAX_MSG_LEN, 0,
                                (struct sockaddr*) &reply_to, &reply_to_len)) < 0)
            // TODO: HANDLE ERROR
            break;
        // timestamp reception
        recv_time_ns = Node::current_time_ns();

        // message is now in buf
        switch (static_cast<Protocol::MSG_T>(buf[0]))
        {
            case Protocol::BEACON:
            {
                // this is a reference node: in the case of beacons, just reply with timestamps
                auto* beacon = (Protocol::MSG_BEACON*) &buf[1];
                Protocol::MSG_BEACON_REPLY reply{};
                memset(&reply, 0, sizeof(Protocol::MSG_BEACON_REPLY));
                reply.seq = beacon->seq;
                reply.beacon_recv_time = recv_time_ns;

                size_t reply_len = sizeof(reply);
                size_t total_len = reply_len + sizeof(Protocol::MSG_T);
                uint8_t reply_buf[total_len];

                reply_buf[0] = Protocol::MSG_T::HANDSHAKE_REPLY;

                reply.reply_send_time = Node::current_time_ns(); // timestamp at the latest possible instant
                memcpy(reinterpret_cast<void*>(reply_buf[1]), &reply, reply_len);

                if ((sent_sz = sendto(this->sock_fd, reply_buf, total_len, 0, (struct sockaddr*) &reply_to,
                                      reply_to_len)) != total_len)
                    // TODO: HANDLE ERROR
                    exit(1);
                break;
            }
            case Protocol::GOODBYE:
            {
                // ack GOODBYE by sending back a GOODBYE_REPLY
                Protocol::MSG_T reply = Protocol::MSG_T::GOODBYE_REPLY;
                if ((sent_sz = sendto(this->sock_fd, &reply, sizeof(reply), 0, (struct sockaddr*) &reply_to,
                                      reply_to_len)) != sizeof(reply))
                    // TODO: HANDLE ERROR
                    exit(1);

                listening = false;
                break;
            }
            default:
                // ignore everything else
                break;
        }
    }


    // shut down
}
