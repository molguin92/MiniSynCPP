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
# include <map>
# include <set>
#include "constraints.h"

namespace MiniSync
{ ;

    class SyncAlgorithm
    {
    protected:
        // constraints
        std::map<std::pair<LowerPoint, HigherPoint>, ConstraintLine> low_constraints;
        std::map<std::pair<LowerPoint, HigherPoint>, ConstraintLine> high_constraints;
        std::set<LowerPoint> low_points;
        std::set<HigherPoint> high_points;

        ConstraintLine current_high;
        ConstraintLine current_low;
        std::pair<LowerPoint, HigherPoint> current_high_pts;
        std::pair<LowerPoint, HigherPoint> current_low_pts;

        struct
        {
            long double value = 0;
            long double error = 0;
        } currentDrift; // relative drift of the clock

        struct
        {
            us_t value{0};
            us_t error{0};
        } currentOffset; // current offset in µseconds

        us_t diff_factor; // difference between current lines
        uint32_t processed_timestamps;

        SyncAlgorithm();

        /*
         * Subclasses need to override this function with their own drift and offset estimation implementation.
         */
        virtual void __recalculateEstimates(const LowerPoint& n_low, const HigherPoint& n_high) = 0;
    public:
        /*
         * Add a new DataPoint and recalculate offset and drift.
         */
        void addDataPoint(us_t To, us_t Tb, us_t Tr);

        /*
         * Get the current estimated relative clock drift.
         */
        long double getDrift();
        long double getDriftError();

        /*
         * Get the current estimated relative clock offset in nanoseconds.
         */
        us_t getOffsetNanoSeconds();
        us_t getOffsetError();

        /*
         * Get the current adjusted time
         */
        std::chrono::time_point<std::chrono::system_clock, us_t> getCurrentAdjustedTime();
    };

    class TinySyncAlgorithm : public SyncAlgorithm
    {
    public:
        TinySyncAlgorithm() = default;
    private:
        void __recalculateEstimates(const LowerPoint& n_low, const HigherPoint& n_high) final;
    };

    class MiniSyncAlgorithm : public SyncAlgorithm
    {
    public:
        MiniSyncAlgorithm() = default;
    private:
        void __recalculateEstimates(const LowerPoint& n_low, const HigherPoint& n_high) final; // TODO: implement
    };
}


#endif //TINYSYNC___MINISYNC_H
