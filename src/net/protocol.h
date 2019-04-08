/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef MINISYNCPP_PROTOCOL_H
#define MINISYNCPP_PROTOCOL_H

#include <cstdint>

namespace MiniSync
{
    namespace Protocol
    {
        // Simple message protocol definition for the sync algorithm

        enum MSG_T : uint8_t
        {
            HANDSHAKE = 0x00,
            BEACON = 0xf0,
            BEACON_REPLY = 0xf1,
            GOODBYE = 0xff
        };

        typedef struct MSG_HANDSHAKE
        {
            uint8_t prot_v_major = 0;
            uint8_t prot_v_minor = 0;
            uint8_t node_mode = -1;
        } MSG_HANDSHAKE;

        typedef struct MSG_BEACON
        {
            uint8_t seq = 0;
            uint64_t send_time = 0;
        } MSG_BEACON;

        typedef struct MSG_BEACON_REPLY
        {
            uint8_t seq = 0;
            uint64_t beacon_send_time = 0;
            uint64_t beacon_recv_time = 0;
            uint64_t reply_send_time = 0;
        } MSG_BEACON_REPLY;

        typedef struct MSG_GOODBYE
        {
            uint8_t ack = 0;
        } MSG_GOODBYE;

    }
}

#endif //MINISYNCPP_PROTOCOL_H
