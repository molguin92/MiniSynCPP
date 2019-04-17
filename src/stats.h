/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef MINISYNCPP_STATS_H
#define MINISYNCPP_STATS_H

#include <cinttypes>
#include <vector>

namespace MiniSynCPP
{
    namespace Stats
    {
        typedef struct Sample
        {
            int64_t offset = 0;
            double offset_error = 0;

            double drift = 0;
            double drift_error = 0;
        } Sample;

        class SyncStats
        {
        private:
            static const uint32_t INIT_VEC_SIZE = 100000;
            std::vector<Sample> samples;

            SyncStats() : samples{INIT_VEC_SIZE}
            {};


        };
    }
}

#endif //MINISYNCPP_STATS_H
