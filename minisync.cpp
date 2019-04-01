/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include <cstdlib>
#include "minisync.h"

float MiniSync::SyncAlgorithm::getDrift()
{
    return this->currentDrift;
}

int64_t MiniSync::SyncAlgorithm::getOffsetNanoSeconds()
{
    return this->currentOffset;
}

uint64_t MiniSync::SyncAlgorithm::getCurrentTimeNanoSeconds()
{
    auto t_now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(t_now).count();
    return static_cast<uint64_t>(now * this->currentDrift) + this->currentOffset;
}

void MiniSync::SyncAlgorithm::addDataPoint(int64_t t_o, int64_t t_b, int64_t t_r)
{
    if (processed_timestamps == 0)
    {
        // first sample
        this->low_1 = std::make_pair(t_b, t_o);
        this->high_1 = std::make_pair(t_b, t_r);
        this->processed_timestamps++;
    }
    else if (processed_timestamps == 1)
    {
        // second sample
        this->low_2 = std::make_pair(t_b, t_o);
        this->high_2 = std::make_pair(t_b, t_r);
        this->processed_timestamps++;

        // now we have two DataPoints, estimate...
        auto L1 = MiniSync::SyncAlgorithm::intersectLine(this->low_1, this->high_2);
        auto L2 = MiniSync::SyncAlgorithm::intersectLine(this->low_2, this->high_1);

        // none of the slopes can be negative
        if (L1.first < 0 || L2.first < 0) throw MiniSync::Exception();

        this->current_L1 = L1;
        this->current_L2 = L2;

        this->currentDrift = (this->current_L2.first + this->current_L1.first) / 2;
        this->currentOffset = static_cast<uint64_t>((this->current_L2.second + this->current_L1.second) / 2);
        this->currentDriftError = (this->current_L2.first - this->current_L1.first) / 2;
        this->currentOffsetError = (this->current_L2.second - this->current_L1.second) / 2;
    }
    else
    {
        // n_th sample, n > 2
        // pass it on to the specific algorithm
        std::pair<uint64_t, uint64_t> n_low = std::make_pair(t_b, t_o);
        std::pair<uint64_t, uint64_t> n_high = std::make_pair(t_b, t_r);

        this->__recalculateEstimates(n_low, n_high);
    }

}

std::pair<long double, long double> MiniSync::SyncAlgorithm::intersectLine(MiniSync::point p1, MiniSync::point p2)
{
    long double M = static_cast<long double>(p1.second - p2.second) / (p1.first - p2.first);
    long double C = p1.second - (M * p1.first);
    return std::make_pair(M, C);
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
void MiniSync::TinySyncAlgorithm::__recalculateEstimates(point& n_low, point& n_high)
{
    // find the tightest bound

}
