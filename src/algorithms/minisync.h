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

    class Point
    {
    public:
        const uint64_t x;
        const uint64_t y;
        Point() = delete;

        Point(uint64_t x, uint64_t y) : x(x), y(y)
        {};

        Point(const Point& o) = default;
    };

    class ConstraintLine
    {
    private:
        long double A;
        long double B;
    public:
        ConstraintLine() = delete;
        ConstraintLine(const Point& p1, const Point& p2);

        long double getA()
        { return this->A; }

        long double getB()
        { return this->B; }

        const Point p1;
        const Point p2;
    };

    class SyncAlgorithm
    {
    protected:
        // constraints
        ConstraintLine* low2high;
        ConstraintLine* high2low;

        Point* init_low;
        Point* init_high;

        struct
        {
            long double value = 0;
            long double error = 0;
        } currentDrift; // relative drift of the clock

        struct
        {
            int64_t value = 0;
            long double error = 0;
        } currentOffset; // current offset in nanoseconds

        long double diff_factor; // difference between current lines
        uint32_t processed_timestamps;

        SyncAlgorithm() :
        init_low(nullptr),
        init_high(nullptr),
        low2high(nullptr),
        high2low(nullptr),
        processed_timestamps(0),
        diff_factor(0)
        {};

        /*
         * Subclasses need to override this function with their own drift and offset estimation implementation.
         */
        virtual void __recalculateEstimates(Point& n_low, Point& n_high) = 0;
    public:
        /*
         * Add a new DataPoint and recalculate offset and drift.
         */
        void addDataPoint(int64_t t_o, int64_t t_b, int64_t t_r);

        /*
         * Get the current estimated relative clock drift.
         */
        long double getDrift();
        long double getDriftError();

        /*
         * Get the current estimated relative clock offset in nanoseconds.
         */
        int64_t getOffsetNanoSeconds();
        long double getOffsetError();

        /*
         * Get the current POSIX timestamp in nanoseconds corrected using the estimated relative clock drift and offset.
         */
        uint64_t getCurrentTimeNanoSeconds();

        virtual ~SyncAlgorithm();
    };

    class TinySyncAlgorithm : public SyncAlgorithm
    {
    public:
        TinySyncAlgorithm() = default;
        ~TinySyncAlgorithm() override = default;
    private:
        void __recalculateEstimates(Point& n_low, Point& n_high) final;
    };

    class MiniSyncAlgorithm : public SyncAlgorithm
    {
    public:
        MiniSyncAlgorithm() = default;
        ~MiniSyncAlgorithm() override = default;
    private:
        void __recalculateEstimates(Point& n_low, Point& n_high) final;
    };
}


#endif //TINYSYNC___MINISYNC_H
