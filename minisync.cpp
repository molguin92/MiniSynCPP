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
        auto bounds = MiniSync::SyncAlgorithm::calculateBounds(this->low_1, this->high_1, this->low_2, this->high_2);
        this->current_A = bounds.first;
        this->current_B = bounds.second;

        this->currentDrift = (this->current_A.second + this->current_A.first) / 2;
        this->currentOffset = static_cast<uint64_t>((this->current_B.second + this->current_B.first) / 2);
        this->currentDriftError = (this->current_A.second - this->current_A.first) / 2;
        this->currentOffsetError = (this->current_B.second - this->current_B.first) / 2;
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

std::pair<std::pair<long double, long double>, std::pair<long double, long double>>
MiniSync::SyncAlgorithm::calculateBounds(std::pair<uint64_t, uint64_t>& low1,
                                         std::pair<uint64_t, uint64_t>& high1,
                                         std::pair<uint64_t, uint64_t>& low2,
                                         std::pair<uint64_t, uint64_t>& high2)
{
    long double A_upper = static_cast<long double>(high2.second - low1.second) / (high2.first - low1.first);
    long double A_lower = static_cast<long double>(high1.second - low2.second) / (high1.first - low2.first);

    // none of the slopes can be negative, as clocks are monotonically increasing:
    if (A_upper < 0 || A_lower < 0)
        throw MiniSync::Exception();

    long double B_lower = low1.second - (A_upper * low1.first);
    long double B_upper = low2.second - (A_lower * low2.first);

    return std::make_pair(std::make_pair(A_lower, A_upper), std::make_pair(B_lower, B_upper));
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
void MiniSync::TinySyncAlgorithm::__recalculateEstimates(std::pair<uint64_t, uint64_t>& n_low,
                                                         std::pair<uint64_t, uint64_t>& n_high)
{
    // find the tightest bound

}
