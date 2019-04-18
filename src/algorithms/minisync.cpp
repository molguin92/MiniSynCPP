/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <cstdlib>
#include <loguru/loguru.hpp>
#include "minisync.h"

long double MiniSync::SyncAlgorithm::getDrift()
{
    return this->currentDrift.value;
}

long double MiniSync::SyncAlgorithm::getDriftError()
{
    return this->currentDrift.error;
}

int64_t MiniSync::SyncAlgorithm::getOffsetNanoSeconds()
{
    return this->currentOffset.value;
}

long double MiniSync::SyncAlgorithm::getOffsetError()
{
    return this->currentOffset.error;
}

uint64_t MiniSync::SyncAlgorithm::getCurrentTimeNanoSeconds()
{
    auto t_now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(t_now).count();
    return static_cast<uint64_t>(now * this->currentDrift.value) + this->currentOffset.value;
}

void MiniSync::SyncAlgorithm::addDataPoint(int64_t t_o, int64_t t_b, int64_t t_r)
{
    if (processed_timestamps == 0)
    {
        // first sample
        this->init_low = new Point(t_b, t_o);
        this->init_high = new Point(t_b, t_r);
        this->processed_timestamps++;
    }
    else if (processed_timestamps == 1)
    {
        // second sample
        // now we can create the lines
        this->low2high = new ConstraintLine(*this->init_low, Point(t_b, t_r));
        this->high2low = new ConstraintLine(*this->init_high, Point(t_b, t_o));
        this->processed_timestamps++;

        long double A_upper = this->low2high->getA();
        long double B_lower = this->low2high->getB();
        long double A_lower = this->high2low->getA();
        long double B_upper = this->high2low->getB();

        this->diff_factor = (A_upper - A_lower) * (B_upper - B_lower);

        this->currentDrift.value = (A_lower + A_upper) / 2;
        this->currentOffset.value = static_cast<uint64_t>((B_lower + B_upper) / 2);
        this->currentDrift.error = (A_upper - A_lower) / 2;
        this->currentOffset.error = (B_upper - B_lower) / 2;
    }
    else
    {
        // n_th sample, n > 2
        // pass it on to the specific algorithm
        auto n_low = Point(t_b, t_o);
        auto n_high = Point(t_b, t_r);
        this->__recalculateEstimates(n_low, n_high);
    }

}

MiniSync::SyncAlgorithm::~SyncAlgorithm()
{
    delete (this->init_high);
    delete (this->init_low);
    delete (this->high2low);
    delete (this->low2high);
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
void MiniSync::TinySyncAlgorithm::__recalculateEstimates(Point& n_low, Point& n_high)
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
    auto* lower_1 = new ConstraintLine(this->low2high->p1, n_high);
    auto* lower_2 = new ConstraintLine(this->high2low->p2, n_high);

    // upper lines (a_lower * x + b_upper)
    auto* upper_1 = new ConstraintLine(this->high2low->p1, n_low);
    auto* upper_2 = new ConstraintLine(this->low2high->p2, n_low);

    ConstraintLine* new_upper = this->high2low;
    ConstraintLine* new_lower = this->low2high;
    auto new_diff = this->diff_factor;

    for (auto* lower: {this->low2high, lower_1, lower_2})
    {
        for (auto* upper: {this->high2low, upper_1, upper_2})
        {
            if (lower == this->low2high && upper == this->high2low) continue;

            auto d = (lower->getA() - upper->getA()) * (upper->getB() - lower->getB());
            if (d < new_diff)
            {
                new_diff = d;
                new_upper = upper;
                new_lower = lower;
            }
        }
    }

    // cleanup
    for (auto* constraint: {this->low2high, this->high2low, lower_1, lower_2, upper_1, upper_2})
    {
        if (constraint == new_lower || constraint == new_upper) continue;
        delete (constraint);
    }

    this->low2high = new_lower;
    this->high2low = new_upper;
    this->diff_factor = new_diff;

    this->currentDrift.value = (new_lower->getA() + new_upper->getA()) / 2;
    this->currentOffset.value = static_cast<uint64_t>((new_lower->getB() + new_upper->getB()) / 2);
    this->currentDrift.error = (new_lower->getA() - new_upper->getA()) / 2;
    this->currentOffset.error = (new_upper->getB() - new_lower->getB()) / 2;
}

MiniSync::ConstraintLine::ConstraintLine(const Point& p1, const Point& p2) : p1(p1), p2(p2)
{
    this->A = static_cast<long double>(p2.y - p1.y) / static_cast<long double>(p2.x - p1.x);

    // slope can't be negative
    if (this->A < 0) throw MiniSync::Exception();

    this->B = static_cast<long double>(p1.y) - (this->A * p1.x);
}
