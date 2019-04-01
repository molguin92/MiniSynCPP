/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef TINYSYNC___MINISYNC_H
#define TINYSYNC___MINISYNC_H

# include <chrono>
# include <tuple>
# include <exception>

namespace MiniSync
{
    template<class Duration> using sys_time = std::chrono::time_point<std::chrono::system_clock, Duration>;
    using sys_nanoseconds = sys_time<std::chrono::nanoseconds>;

    class Exception : public std::exception
    {
    public:
        const char* what() const noexcept final
        {
            return "Error in time synchronization, got invalid timestamp!";
        }
    };

    class SyncAlgorithm
    {
    protected:
        // constraints
        std::pair<uint64_t, uint64_t> low_1;
        std::pair<uint64_t, uint64_t> low_2;
        std::pair<uint64_t, uint64_t> high_1;
        std::pair<uint64_t, uint64_t> high_2;


        long double currentDrift; // relative drift of the clock
        int64_t currentOffset; // current offset in nanoseconds

        long double currentDriftError;
        long double currentOffsetError;

        std::pair<long double, long double> current_A;
        std::pair<long double, long double> current_B;

        uint32_t processed_timestamps;

        SyncAlgorithm() :
        currentDrift(1.0),
        currentOffset(0),
        currentDriftError(0.0),
        currentOffsetError(0.0),
        low_1(0, 0),
        low_2(0, 0),
        high_1(0, 0),
        high_2(0, 0),
        current_A(0, 0),
        current_B(0, 0),
        processed_timestamps(0)
        {};

        /*
         * Subclasses need to override this function with their own drift and offset estimation implementation.
         */
        virtual void __recalculateEstimates(std::pair<uint64_t, uint64_t>& n_low,
                                            std::pair<uint64_t, uint64_t>& n_high) = 0;

        /*
         * Calculates the bounds for A and B given two DataPoints
         */
        static std::pair<std::pair<long double, long double>, std::pair<long double, long double>>
        calculateBounds(std::pair<uint64_t, uint64_t>& low1,
                        std::pair<uint64_t, uint64_t>& high1,
                        std::pair<uint64_t, uint64_t>& low2,
                        std::pair<uint64_t, uint64_t>& high2);
    public:
        /*
         * Add a new DataPoint and recalculate offset and drift.
         */
        void addDataPoint(int64_t t_o, int64_t t_b, int64_t t_r);

        /*
         * Get the current estimated relative clock drift.
         */
        float getDrift();

        /*
         * Get the current estimated relative clock offset in nanoseconds.
         */
        int64_t getOffsetNanoSeconds();

        /*
         * Get the current POSIX timestamp in nanoseconds corrected using the estimated relative clock drift and offset.
         */
        uint64_t getCurrentTimeNanoSeconds();
    };

    class TinySyncAlgorithm : public SyncAlgorithm
    {
    public:
        TinySyncAlgorithm() = default;
    private:
        void __recalculateEstimates(std::pair<uint64_t, uint64_t>& n_low,
                                    std::pair<uint64_t, uint64_t>& n_high) override;
    };

    class MiniSyncAlgorithm : public SyncAlgorithm
    {
    public:
        MiniSyncAlgorithm() = default;
    private:
        void __recalculateEstimates(std::pair<uint64_t, uint64_t>& n_low,
                                    std::pair<uint64_t, uint64_t>& n_high) override;
    };
}


#endif //TINYSYNC___MINISYNC_H
