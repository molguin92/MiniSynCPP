/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef MINISYNCPP_PROTOCOL_H
#define MINISYNCPP_PROTOCOL_H

#include <cinttypes>
#include "protocol.pb.h"

namespace MiniSync
{
    namespace Protocol
    {
        // TODO: Parametrize version numbers?
        static const uint8_t VERSION_MAJOR = 0;
        static const uint8_t VERSION_MINOR = 1;

        // Maximum message length corresponds to the maximum UDP datagram size.
        static const uint32_t MAX_MSG_LEN = 65507;
    }
}

#endif //MINISYNCPP_PROTOCOL_H
