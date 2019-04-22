/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <cstdlib>
#include <loguru/loguru.hpp>
#include <algorithm>
#include "minisync.h"

long double MiniSync::SyncAlgorithm::getDrift()
{
    return this->currentDrift.value;
}

long double MiniSync::SyncAlgorithm::getDriftError()
{
    return this->currentDrift.error;
}

MiniSync::us_t MiniSync::SyncAlgorithm::getOffsetNanoSeconds()
{
    return this->currentOffset.value;
}

MiniSync::us_t MiniSync::SyncAlgorithm::getOffsetError()
{
    return this->currentOffset.error;
}

std::chrono::time_point<std::chrono::system_clock, MiniSync::us_t> MiniSync::SyncAlgorithm::getCurrentAdjustedTime()
{
    auto t_now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::time_point<std::chrono::system_clock, us_t>{
    this->currentDrift.value * t_now +
    this->currentOffset.value
    };
}

void MiniSync::SyncAlgorithm::addDataPoint(us_t To, us_t Tb, us_t Tr)
{

    if (processed_timestamps == 0)
    {
        // first sample
        ++this->processed_timestamps;
        this->low_points.emplace(Tb, To);
        this->high_points.emplace(Tb, Tr);
    }
    else
    {
        // n_th sample, n >= 1
        // pass it on to the specific algorithm
        ++this->processed_timestamps;
        this->__recalculateEstimates(LowerPoint(Tb, To), HigherPoint(Tb, Tr));
    }

}

MiniSync::SyncAlgorithm::SyncAlgorithm() :
processed_timestamps(0),
diff_factor(std::numeric_limits<long double>::max())
{
}

/*
 * Update estimate based on the constraints we have stored.
 *
 * Considering
 *
 * constraint1 = {tol, tbl, trl}
 * constraint2 = {tor, tbr, trr}
 *
 * We have four points to build two linear equations:
 * {tbl, tol} -> {tbr, trr}: (A_upper, B_lower)
 * {tbl, trl} -> {tbr, tor}: (A_lower, B_upper)
 *
 * Where
 * A_upper = (trr - tol)/(tbr - tbl)
 * A_lower = (tor - trl)/(tbr - tbl)
 * B_upper = trl - A_lower * tbl
 * B_lower = tol - A_upper * tbl
 *
 * Using these bounds, we can estimate the Drift and Offset as
 *
 * Drift = (A_upper + A_lower)/2
 * Offset = (B_upper + B_lower)/2
 * Drift_Error = (A_upper - A_lower)/2
 * Offset_Error = (B_upper - B_lower)/2
 */
void MiniSync::TinySyncAlgorithm::__recalculateEstimates(const LowerPoint& n_low, const HigherPoint& n_high)
{
    // assume timestamps come in time order
    //
    // find the tightest bound
    // only need to compare the lines
    // l1 -> h2 (already have)
    // l2 -> h1 (already have)
    // l1 -> n_h
    // l2 -> n_h
    // n_l -> h1
    // n_l -> h2
    //
    // find minimum (a_upper - a_lower)(b_upper - b_lower)

    // lower lines (a_upper * x + b_lower)

    // calculate new possible constraints
    for (const auto& point: this->low_points)
        this->low_constraints.emplace(std::make_pair(point, n_high), ConstraintLine(point, n_high));
    for (const auto& point: this->high_points)
        this->high_constraints.emplace(std::make_pair(n_low, point), ConstraintLine(n_low, point));

    // add new points to collection
    this->low_points.insert(n_low);
    this->high_points.insert(n_high);

    us_t tmp_diff;
    ConstraintLine tmp_low;
    ConstraintLine tmp_high;
    for (const auto& iter_low: this->low_constraints)
    {
        for (const auto& iter_high: this->high_constraints)
        {
            tmp_low = iter_low.second;
            tmp_high = iter_high.second;

            tmp_diff = (tmp_low.getA() - tmp_high.getA()) * (tmp_high.getB() - tmp_low.getB());
            if (tmp_diff < this->diff_factor)
            {
                this->diff_factor = tmp_diff;

                this->current_low = tmp_low;
                this->current_low_pts = iter_low.first;

                this->current_high = tmp_high;
                this->current_high_pts = iter_high.first;
            }
        }
    }

    // cleanup
    for (auto iter = low_points.begin(); iter != low_points.end();)
    {
        if (current_low_pts.first != *iter && current_high_pts.first != *iter)
            iter = low_points.erase(iter);
        else
            ++iter;
    }

    for (auto iter = high_points.begin(); iter != high_points.end();)
    {
        if (current_low_pts.second != *iter && current_high_pts.second != *iter)
            iter = high_points.erase(iter);
        else
            ++iter;
    }

    auto tmp_low_cons = std::move(this->low_constraints);
    auto tmp_high_cons = std::move(this->high_constraints);
    for (auto h_point: high_points)
    {
        for (auto l_point: low_points)
        {
            auto pair = std::make_pair(l_point, h_point);
            if (tmp_low_cons.count(pair) > 0)
                this->low_constraints.emplace(pair, tmp_low_cons.at(pair));
            else if (tmp_high_cons.count(pair) > 0)
                this->high_constraints.emplace(pair, tmp_high_cons.at(pair));
        }
    }

    this->currentDrift.value = (current_low.getA() + current_high.getA()) / 2;
    this->currentOffset.value = (current_low.getB() + current_high.getB()) / 2;
    this->currentDrift.error = (current_low.getA() - current_high.getA()) / 2;
    this->currentOffset.error = (current_high.getB() - current_low.getB()) / 2;

    CHECK_GE_F(this->currentDrift.value, 0, "Drift must be >=0 for monotonically increasing clocks...");
}


