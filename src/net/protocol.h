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

        // TODO: Parametrize version numbers?
        static const uint8_t VERSION_MAJOR = 0;
        static const uint8_t VERSION_MINOR = 1;

        // Maximum message length corresponds to the maximum UDP datagram size.
        static const uint32_t MAX_MSG_LEN = 65507;

        enum MSG_T : uint8_t
        {
            HANDSHAKE = 0x00,
            HANDSHAKE_REPLY = 0x01,
            BEACON = 0x10,
            BEACON_REPLY = 0x11,
            GOODBYE = 0xf0,
            GOODBYE_REPLY = 0xff
        };

        enum HANDSHAKE_STATUS : uint8_t
        {
            SUCCESS = 0x00,
            VERSION_MISMATCH = 0xf0,
            MODE_MISMATCH = 0x0f1
        };

        typedef struct MSG_HANDSHAKE
        {
            uint8_t prot_v_major = 0;
            uint8_t prot_v_minor = 0;
            uint8_t node_mode = -1;
        } MSG_HANDSHAKE;

        typedef struct MSG_HANDSHAKE_REPLY
        {
            HANDSHAKE_STATUS status;
        } MSG_HANDSHAKE_REPLY;

        typedef struct MSG_BEACON
        {
            uint8_t seq = 0;
        } MSG_BEACON;

        typedef struct MSG_BEACON_REPLY
        {
            uint8_t seq = 0;
            uint64_t beacon_recv_time = 0;
            uint64_t reply_send_time = 0;
        } MSG_BEACON_REPLY;
    }
}

#endif //MINISYNCPP_PROTOCOL_H
