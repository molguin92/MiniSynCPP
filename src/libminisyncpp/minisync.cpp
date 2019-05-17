/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <cstdlib>
#include <algorithm>

#ifdef LIBMINISYNCPP_LOGURU_ENABLE

#include <loguru/loguru.hpp>

#endif

#include "minisync.h"

/*
 * Get the current relative drift of the clock.
 */
long double MiniSync::Algorithms::Base::getDrift()
{
    return this->currentDrift.value;
}

/*
 * Get the current (one-sided) error of the relative clock drift.
 */
long double MiniSync::Algorithms::Base::getDriftError()
{
    return this->currentDrift.error;
}

/*
 * Get the current relative offset of the clock.
 */
MiniSync::us_t MiniSync::Algorithms::Base::getOffset()
{
    return this->currentOffset.value;
}

/*
 * Get the current (one-sided) error of the relative clock offset.
 */
MiniSync::us_t MiniSync::Algorithms::Base::getOffsetError()
{
    return this->currentOffset.error;
}

/*
 * Get the current time as a std::chrono::time_point, adjusted using the current relative offset and drift.
 */
std::chrono::time_point<std::chrono::system_clock, MiniSync::us_t> MiniSync::Algorithms::Base::getCurrentAdjustedTime()
{
    auto t_now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::time_point<std::chrono::system_clock, us_t>{
        this->currentDrift.value * t_now +
        this->currentOffset.value
    };
}

/*
 * Adds a data point to the algorithm and recalculates the drift and offset estimates.
 */
void MiniSync::Algorithms::Base::addDataPoint(us_t To, us_t Tb, us_t Tr)
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
        // TODO: extract this call from this method?
        this->__recalculateEstimates();
    }
}

MiniSync::Algorithms::Base::Base() :
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
void MiniSync::Algorithms::Base::__recalculateEstimates()
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
                this->low_constraint_pts = iter_low.first;

                this->current_high = tmp_high;
                this->high_constraint_pts = iter_high.first;
            }
        }
    }

    this->cleanup();

#ifdef LIBMINISYNCPP_LOGURU_ENABLE
    CHECK_NE_F(current_low.get(), nullptr, "current_low is null!");
    CHECK_NE_F(current_high.get(), nullptr, "current_high is null!");
#else
    if (current_low.get() == nullptr)
        throw std::runtime_error("current_low is null!");
    if (current_high.get() == nullptr)
        throw std::runtime_error("current_high is null!");
#endif

    this->currentDrift.value = (current_low->getA() + current_high->getA()) / 2;
    this->currentOffset.value = (current_low->getB() + current_high->getB()) / 2;
    this->currentDrift.error = (current_low->getA() - current_high->getA()) / 2;
    this->currentOffset.error = (current_high->getB() - current_low->getB()) / 2;

#ifdef LIBMINISYNCPP_LOGURU_ENABLE
    CHECK_GE_F(this->currentDrift.value,
               0,
               "Drift must be >=0 for monotonically increasing clocks... (actual value: %Lf)",
               this->currentDrift.value);
#else
    if (this->currentDrift.value <= 0)
        throw std::runtime_error("Drift must be >=0 for monotonically increasing clocks...");
#endif
}

MiniSync::LPointPtr MiniSync::Algorithms::MiniSync::addLowPoint(us_t Tb, us_t To)
{
    auto lp = Base::addLowPoint(Tb, To);
    // calculate the slopes for the minisync algo
    std::pair<LPointPtr, LPointPtr> pair;
    long double M;
    for (LPointPtr olp: this->low_points)
    {
        if (lp->getX() == olp->getX()) continue; //avoid division by 0...

        // hp is always the newest pointer, put it as the second element of the pair always
        pair = std::make_pair(olp, lp);
        M = (lp->getY() - olp->getY()) / (lp->getX() - olp->getX());
        low_slopes.emplace(pair, M);
    }

    return lp;
}

MiniSync::HPointPtr MiniSync::Algorithms::MiniSync::addHighPoint(us_t Tb, us_t Tr)
{
    auto hp = Base::addHighPoint(Tb, Tr);
    // calculate the slopes for the minisync algo
    std::pair<HPointPtr, HPointPtr> pair;
    long double M;
    for (HPointPtr ohp: this->high_points)
    {
        if (hp->getX() == ohp->getX()) continue; //avoid division by 0...

        // hp is always the newest pointer, put it as the second element of the pair always
        pair = std::make_pair(ohp, hp);
        M = (hp->getY() - ohp->getY()) / (hp->getX() - ohp->getX());
        high_slopes.emplace(pair, M);
    }

    return hp;
}

MiniSync::LPointPtr MiniSync::Algorithms::Base::addLowPoint(us_t Tb, us_t To)
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

MiniSync::HPointPtr MiniSync::Algorithms::Base::addHighPoint(us_t Tb, us_t Tr)
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

/*
 * Helper instance method to add a constraint linear equation given a lower-bound and an upper-bound point.
 */
bool MiniSync::Algorithms::Base::addConstraint(LPointPtr lp, HPointPtr hp)
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

/*
 * Removes un-used data points from the algorithm internal storage.
 */
void MiniSync::Algorithms::TinySync::cleanup()
{
    // cleanup
    for (auto iter = low_points.begin(); iter != low_points.end();)
    {
        if (low_constraint_pts.first != *iter && high_constraint_pts.first != *iter)
            iter = low_points.erase(iter);
        else
            ++iter;
    }

    for (auto iter = high_points.begin(); iter != high_points.end();)
    {
        if (low_constraint_pts.second != *iter && high_constraint_pts.second != *iter)
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

/*
 * Removes un-used data points from the algorithm internal storage.
 */
void MiniSync::Algorithms::MiniSync::cleanup()
{
    // cleanup
    for (auto iter_j = low_points.begin(); iter_j != low_points.end();)
    {
        if (low_constraint_pts.first != *iter_j && high_constraint_pts.first != *iter_j)
        {
            // low point is not one of the current constraint low points
            // now we compare the slopes with each other point to see if we store it for future use or not
            // if current point is Aj, compare each M(Ai, Aj) with M(Aj, Ak)
            // store point Aj only iff there exists
            // M(Ai, Aj) > M(Aj, Ak) for 0 <= i < j < k <= total number of points.
            bool store = false;
            for (auto iter_i = low_points.begin(); iter_i != iter_j; ++iter_i)
            {
                for (auto iter_k = std::next(iter_j, 1); iter_k != low_points.end(); ++iter_k)
                {
                    if (low_slopes.at(std::make_pair(*iter_i, *iter_j))
                        > low_slopes.at(std::make_pair(*iter_j, *iter_k)))
                    {
                        store = true;
                        break;
                    }
                }

                if (store) break;
            }

            if (!store)
            {
                // delete all associated slopes then delete the point
                for (auto iter_slope = low_slopes.begin(); iter_slope != low_slopes.end();)
                {
                    if (iter_slope->first.first == *iter_j || iter_slope->first.second == *iter_j)
                        iter_slope = low_slopes.erase(iter_slope);
                    else ++iter_slope;
                }
                iter_j = low_points.erase(iter_j);
            }
            else
                ++iter_j;
        }
        else
            ++iter_j;
    }

    // done same thing for high points
    for (auto iter_j = high_points.begin(); iter_j != high_points.end();)
    {
        if (low_constraint_pts.second != *iter_j && high_constraint_pts.second != *iter_j)
        {
            // high point is not one of the current constraint high points
            // now we compare the slopes with each other point to see if we store it for future use or not
            // if current point is Aj, compare each M(Ai, Aj) with M(Aj, Ak)
            // store point Aj only iff there exists
            // M(Ai, Aj) < M(Aj, Ak) for 0 <= i < j < k <= total number of points. This is the inverse condition as
            // the low points.
            bool store = false;
            for (auto iter_i = high_points.begin(); iter_i != iter_j; ++iter_i)
            {
                for (auto iter_k = std::next(iter_j, 1); iter_k != high_points.end(); ++iter_k)
                {
                    try
                    {
                        if (high_slopes.at(std::make_pair(*iter_i, *iter_j))
                            < high_slopes.at(std::make_pair(*iter_j, *iter_k)))
                        {
                            store = true;
                            break;
                        }
                    }
                    catch (std::out_of_range& e)
                    {
#ifdef LOGURU_ENABLE
                        DLOG_F(WARNING, "iter_i: (%Lf, %Lf)",
                               iter_i->get()->getX().count(),
                               iter_i->get()->getY().count());
                        DLOG_F(WARNING, "iter_j: (%Lf, %Lf)",
                               iter_j->get()->getX().count(),
                               iter_j->get()->getY().count());
                        DLOG_F(WARNING, "iter_k: (%Lf, %Lf)",
                               iter_k->get()->getX().count(),
                               iter_k->get()->getY().count());
#endif
                        throw;
                    }
                }

                if (store) break;
            }

            if (!store)
            {
                // delete all associated slopes then delete the point
                for (auto iter_slope = high_slopes.begin(); iter_slope != high_slopes.end();)
                {
                    if (iter_slope->first.first == *iter_j || iter_slope->first.second == *iter_j)
                        iter_slope = high_slopes.erase(iter_slope);
                    else ++iter_slope;
                }
                iter_j = high_points.erase(iter_j);
            }
            else
                ++iter_j;
        }
        else
            ++iter_j;
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

size_t std::hash<MiniSync::Point>::operator()(const MiniSync::Point& point) const
{
    return std::hash<long double>()(point.getX().count() + point.getY().count());
}
