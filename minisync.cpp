/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© Manuel Olguín Muñoz 2019
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "minisync.h"

MiniSync::SyncAlgorithm::SyncAlgorithm(): currentDrift(1.0), currentOffset(0.0)
{}

float MiniSync::SyncAlgorithm::getDrift()
{
    return this->currentDrift;
}

float MiniSync::SyncAlgorithm::getOffset()
{
    return this->currentOffset;
}


