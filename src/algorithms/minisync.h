/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef TINYSYNC___MINISYNC_H
#define TINYSYNC___MINISYNC_H

#include <chrono>
#include <tuple>
#include <exception>
#include <unordered_map>
#include <set>
#include <memory>
#include "constraints.h"

namespace MiniSync
{
    using LPointPtr = std::shared_ptr<LowPoint>;
    using HPointPtr = std::shared_ptr<HighPoint>;
    using ConstraintPtr = std::shared_ptr<ConstraintLine>;

    // some hash specializations we'll need
    struct ppair_hash
    {
        std::size_t operator()(const std::pair<std::shared_ptr<Point>, std::shared_ptr<Point>>& pair) const
        {
            return std::hash<MiniSync::Point>()(*pair.first) ^ std::hash<MiniSync::Point>()(*pair.second);
        }
    };

    // some comparison operations for sets of pointers to Points
    struct lppoint_compare
    {
        bool operator()(const LPointPtr& lhs, const LPointPtr& rhs)
        {
            return *lhs < *rhs;
        }
    };

    struct hppoint_compare
    {
        bool operator()(const HPointPtr& lhs, const HPointPtr& rhs)
        {
            return *lhs < *rhs;
        }
    };

    class SyncAlgorithm
    {
    protected:
        // constraints
        std::unordered_map<std::pair<LPointPtr, HPointPtr>, ConstraintPtr, ppair_hash> low_constraints;
        std::unordered_map<std::pair<LPointPtr, HPointPtr>, ConstraintPtr, ppair_hash> high_constraints;
        std::set<LPointPtr, lppoint_compare> low_points;
        std::set<HPointPtr, hppoint_compare> high_points;

        ConstraintPtr current_high;
        ConstraintPtr current_low;
        std::pair<LPointPtr, HPointPtr> high_constraint_pts;
        std::pair<LPointPtr, HPointPtr> low_constraint_pts;

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

        void __recalculateEstimates();

        /*
         * Subclasses need to override this function with their own cleanup method.
         */

        virtual void __cleanup() = 0;

        /*
         * Add a new low point and calculate associated constraints
         */
        virtual LPointPtr addLowPoint(us_t Tb, us_t To);
        virtual HPointPtr addHighPoint(us_t Tb, us_t Tr);
        virtual bool addConstraint(LPointPtr lp, HPointPtr hp);
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
        us_t getOffset();
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
        void __cleanup() final;
    };

    class MiniSyncAlgorithm : public SyncAlgorithm
    {
    public:
        MiniSyncAlgorithm() = default;
    private:
        std::unordered_map<std::pair<LPointPtr, LPointPtr>, long double, ppair_hash> low_slopes;
        std::unordered_map<std::pair<HPointPtr, HPointPtr>, long double, ppair_hash> high_slopes;
        void __cleanup() final;
        LPointPtr addLowPoint(us_t Tb, us_t To) final;
        HPointPtr addHighPoint(us_t Tb, us_t Tr) final;
    };
}


#endif //TINYSYNC___MINISYNC_H
