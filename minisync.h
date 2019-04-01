/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef TINYSYNC___MINISYNC_H
#define TINYSYNC___MINISYNC_H

# include <chrono>

namespace MiniSync
{
    template<class Duration>
    using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
    using sys_nanoseconds = sys_time<std::chrono::nanoseconds>;

    struct DataPoint
    {
    public:
        sys_nanoseconds t_o;
        sys_nanoseconds t_br;
        sys_nanoseconds t_bt;
        sys_nanoseconds t_r;
    };

    class SyncAlgorithm
    {
    protected:
        float currentDrift; // relative drift of the clock
        std::chrono::nanoseconds currentOffset; // current offset in nanoseconds
        SyncAlgorithm();
    public:
        virtual void
        addDataPoint(sys_nanoseconds t_o, sys_nanoseconds t_br, sys_nanoseconds t_bt, sys_nanoseconds t_r) = 0;
        float getDrift();
        std::chrono::nanoseconds getOffset();
        sys_nanoseconds getCurrentTimeNanoseconds();
    };

    class TinySyncAlgorithm : public SyncAlgorithm
    {
    };

    class MiniSyncAlgorithm : public SyncAlgorithm
    {
    };
}


#endif //TINYSYNC___MINISYNC_H
