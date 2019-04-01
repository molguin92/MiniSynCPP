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
    template<class Duration> using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
    using sys_nanoseconds = sys_time<std::chrono::nanoseconds>;

    typedef struct
    {
        uint64_t to;
        uint64_t tb;
        uint64_t tr;
    } DataPoint;

    class SyncAlgorithm
    {
    protected:
        // constraints
        DataPoint* ldp;
        DataPoint* rdp;

        long double currentDrift; // relative drift of the clock
        uint64_t currentOffset; // current offset in nanoseconds

        long double currentDriftError;
        long double currentOffsetError;

        SyncAlgorithm() :
        currentDrift(1.0),
        currentOffset(0),
        currentDriftError(0.0),
        currentOffsetError(0.0),
        ldp(nullptr),
        rdp(nullptr)
        {};

        void updateEstimate();
    public:
        /*
         * Add a new DataPoint and recalculate offset and drift.
         */
        virtual void addDataPoint(uint64_t t_o, uint64_t t_b, uint64_t t_r) = 0;

        /*
         * Get the current estimated relative clock drift.
         */
        float getDrift();

        /*
         * Get the current estimated relative clock offset in nanoseconds.
         */
        uint64_t getOffsetNanoSeconds();

        /*
         * Get the current POSIX timestamp in nanoseconds corrected using the estimated relative clock drift and offset.
         */
        uint64_t getCurrentTimeNanoSeconds();
    };

    class TinySyncAlgorithm : public SyncAlgorithm
    {
    public:
        TinySyncAlgorithm() = default;
        void addDataPoint(uint64_t t_o, uint64_t t_b, uint64_t t_r) override;
    };

    class MiniSyncAlgorithm : public SyncAlgorithm
    {
    public:
        MiniSyncAlgorithm() = default;
        void addDataPoint(uint64_t t_o, uint64_t t_b, uint64_t t_r) override;
    };
}


#endif //TINYSYNC___MINISYNC_H
