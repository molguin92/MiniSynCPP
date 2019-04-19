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
#include <string>

namespace MiniSync
{
    namespace Stats
    {
        typedef struct Sample
        {
            long double current_timestamp = 0;
            long double offset = 0;
            long double offset_error = 0;
            long double drift = 0;
            long double drift_error = 0;
        } Sample;

        class SyncStats
        {
        private:
            static const uint32_t INIT_VEC_SIZE = 100000;
            std::vector<Sample> samples;

        public:
            explicit SyncStats(uint32_t init_capacity = INIT_VEC_SIZE) : samples{init_capacity}
            {};

            ~SyncStats() = default;

            void add_sample(long double offset,
                            long double offset_error,
                            long double drift,
                            long double drift_error);

            uint32_t write_csv(const std::string& path);


        };
    }
}

#endif //MINISYNCPP_STATS_H
