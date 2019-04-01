/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "minisync.h"

MiniSync::SyncAlgorithm::SyncAlgorithm() : currentDrift(1.0), currentOffset(0)
{}

float MiniSync::SyncAlgorithm::getDrift()
{
    return this->currentDrift;
}

std::chrono::nanoseconds MiniSync::SyncAlgorithm::getOffset()
{
    return this->currentOffset;
}

MiniSync::sys_nanoseconds MiniSync::SyncAlgorithm::getCurrentTimeNanoseconds()
{
    sys_nanoseconds now = std::chrono::system_clock::now();

    uint64_t
    n_now = static_cast<uint64_t>(now.time_since_epoch().count() * this->currentDrift) + this->currentOffset.count();

    return sys_time<std::chrono::nanoseconds>(std::chrono::nanoseconds(n_now));
}




