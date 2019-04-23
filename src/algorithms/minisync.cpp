/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <cstdlib>
#include <loguru/loguru.hpp>
#include "minisync.h"
#include <algorithm>

long double MiniSync::SyncAlgorithm::getDrift()
{
    return this->currentDrift.value;
}

long double MiniSync::SyncAlgorithm::getDriftError()
{
    return this->currentDrift.error;
}

MiniSync::us_t MiniSync::SyncAlgorithm::getOffset()
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
    // add points to internal storage
    //this->low_points.insert(std::make_shared<LowPoint>(Tb, To));
    // this->high_points.insert(std::make_shared<HighPoint>(Tb, Tr));

    this->addLowPoint(Tb, To);
    this->addHighPoint(Tb, Tr);
    ++this->processed_timestamps;

    if (processed_timestamps > 1)
    {
        // n_th sample, n >= 1
        // pass it on to the specific algorithm
        this->__recalculateEstimates();
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
void MiniSync::SyncAlgorithm::__recalculateEstimates()
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

    us_t tmp_diff;
    for (const auto& iter_low: this->low_constraints)
    {
        for (const auto& iter_high: this->high_constraints)
        {
            ConstraintPtr tmp_low = iter_low.second;
            ConstraintPtr tmp_high = iter_high.second;

            tmp_diff = (tmp_low->getA() - tmp_high->getA()) * (tmp_high->getB() - tmp_low->getB());
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

    this->__cleanup();

    CHECK_NE_F(current_low.get(), nullptr, "current_low is null!");
    CHECK_NE_F(current_high.get(), nullptr, "current_high is null!");

    this->currentDrift.value = (current_low->getA() + current_high->getA()) / 2;
    this->currentOffset.value = (current_low->getB() + current_high->getB()) / 2;
    this->currentDrift.error = (current_low->getA() - current_high->getA()) / 2;
    this->currentOffset.error = (current_high->getB() - current_low->getB()) / 2;

    CHECK_GE_F(this->currentDrift.value, 0, "Drift must be >=0 for monotonically increasing clocks...");
}

MiniSync::LPointPtr MiniSync::SyncAlgorithm::addLowPoint(MiniSync::us_t Tb, MiniSync::us_t To)
{
    auto lp = std::make_shared<LowPoint>(Tb, To);
    std::pair<LPointPtr, HPointPtr> pair;

    // calculate new constraints
    for (HPointPtr hp: this->high_points)
    {
        this->addConstraint(lp, hp);
    }

    this->low_points.insert(lp);
    return lp; // return a copy of the created pointer.
}

MiniSync::HPointPtr MiniSync::SyncAlgorithm::addHighPoint(MiniSync::us_t Tb, MiniSync::us_t Tr)
{
    auto hp = std::make_shared<HighPoint>(Tb, Tr);

    // calculate new constraints
    for (LPointPtr lp: this->low_points)
    {
        this->addConstraint(lp, hp);
    }

    this->high_points.insert(hp);
    return hp; // return a copy of the created pointer.
}

bool MiniSync::SyncAlgorithm::addConstraint(MiniSync::LPointPtr lp, MiniSync::HPointPtr hp)
{
    auto pair = std::make_pair(lp, hp);

    if (lp->getX() == hp->getX()) return false;
    else if (lp->getX() < hp->getX() && this->low_constraints.count(pair) == 0)
    {
        this->low_constraints.emplace(pair, std::make_shared<ConstraintLine>(*lp, *hp));
        return true;
    }
    else if (lp->getX() > hp->getX() && this->high_constraints.count(pair) == 0)
    {
        this->high_constraints.emplace(pair, std::make_shared<ConstraintLine>(*lp, *hp));
        return true;
    }

    return false;
}


void MiniSync::TinySyncAlgorithm::__cleanup()
{
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
                this->low_constraints[pair] = tmp_low_cons.at(pair);
            else if (tmp_high_cons.count(pair) > 0)
                this->high_constraints[pair] = tmp_high_cons.at(pair);
        }
    }
}

void MiniSync::MiniSyncAlgorithm::__cleanup()
{
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
}

MiniSync::LPointPtr MiniSync::MiniSyncAlgorithm::addLowPoint(MiniSync::us_t Tb, MiniSync::us_t To)
{
    auto lp = SyncAlgorithm::addLowPoint(Tb, To);
    // calculate the slopes for the minisync algo
    std::pair<LPointPtr, LPointPtr> pair;
    long double M;
    for (LPointPtr olp: this->low_points)
    {
        // lp is always the newest pointer, put it at the end
        pair = std::make_pair(olp, lp);
        M = (lp->getY() - olp->getY()) / (lp->getX() - olp->getX());
        low_slopes.emplace(pair, M);
    }
}

size_t std::hash<MiniSync::Point>::operator()(const MiniSync::Point& point) const
{
    return std::hash<long double>()(point.getX().count() + point.getY().count());
}
