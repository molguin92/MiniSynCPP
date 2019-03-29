//
// Created by molguin on 2019-03-29.
//

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


