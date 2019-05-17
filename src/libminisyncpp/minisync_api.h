/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/
#ifndef MINISYNCPP_MINISYNC_API_H
#define MINISYNCPP_MINISYNC_API_H

#include <chrono>
#include <memory>

namespace MiniSync
{
    typedef std::chrono::duration<long double, std::chrono::microseconds::period> us_t;

    namespace API
    {
        class Algorithm
        {
        public:
            /*
             * Add a new DataPoint and recalculate offset and drift.
             */
            virtual void addDataPoint(us_t To, us_t Tb, us_t Tr) = 0;

            /*
             * Get the current estimated relative clock drift.
             */
            virtual long double getDrift() = 0;
            virtual long double getDriftError() = 0;

            /*
             * Get the current estimated relative clock offset in nanoseconds.
             */
            virtual us_t getOffset() = 0;
            virtual us_t getOffsetError() = 0;

            /*
             * Get the current adjusted time
             */
            virtual std::chrono::time_point<std::chrono::system_clock, us_t> getCurrentAdjustedTime() = 0;
        };

        namespace Factory
        {
            std::shared_ptr<MiniSync::API::Algorithm> createTinySync();
            std::shared_ptr<MiniSync::API::Algorithm> createMiniSync();
        }
    }
}

#endif //MINISYNCPP_MINISYNC_API_H
