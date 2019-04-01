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

uint64_t MiniSync::SyncAlgorithm::getOffsetNanoSeconds()
{
    return this->currentOffset;
}

uint64_t MiniSync::SyncAlgorithm::getCurrentTimeNanoSeconds()
{
    auto t_now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(t_now).count();
    return static_cast<uint64_t>(now * this->currentDrift) + this->currentOffset;
}

/*
     * Update estimate based on the constraints we have stored.
     *
     * Considering
     *
     * ldp = {tol, tbl, trl}
     * rdp = {tor, tbr, trr}
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
void MiniSync::SyncAlgorithm::updateEstimate()
{
    long double A_upper = static_cast<long double>(rdp->tr - ldp->to) / (rdp->tb - ldp->tb);
    long double A_lower = static_cast<long double>(rdp->to - ldp->tr) / (rdp->tb - ldp->tb);
    long double B_upper = ldp->tr - (A_lower * ldp->tb);
    long double B_lower = ldp->to - (A_upper * ldp->tb);

    this->currentDrift = (A_upper + A_lower) / 2;
    this->currentOffset = static_cast<uint64_t>((B_upper + B_lower) / 2);
    this->currentDriftError = (A_upper - A_lower) / 2;
    this->currentOffsetError = (B_upper - B_lower) / 2;
}

void MiniSync::TinySyncAlgorithm::addDataPoint(uint64_t t_o, uint64_t t_b, uint64_t t_r)
{
    /*
     * (t_o, t_b, t_r) form a data-point which effectively limits the possible values of parameters a12 and b12 in
     * t1 = a12*t2 + b12
     *
     * t_o < a12*t_b + b12
     * t_r > a12*t_b + b12
     */

    if (ldp == nullptr)
    {
        // first sample
        this->ldp = new DataPoint{t_o, t_b, t_r};
        return;
    }
    else if (rdp == nullptr)
    {
        // second sample
        if (t_o > ldp->to && t_b > ldp->tb)
            rdp = new DataPoint{t_o, t_b, t_r};
        else if (t_o < ldp->to && t_b < ldp->tb)
        {
            rdp = ldp;
            ldp = new DataPoint{t_o, t_b, t_r};
        }
        else
        {
            // TODO: handle error, this shouldn't happen
            exit(-1);
        }
    }
    else
    {
        if (t_o > rdp->to && t_b > rdp->tb)
        {
            // replace right constraint
            delete rdp;
            rdp = new DataPoint{t_o, t_b, t_r};
        }
        else if (t_o < ldp->to && t_b < ldp->tb)
        {
            // replace left constraint
            delete ldp;
            ldp = new DataPoint{t_o, t_b, t_r};
        }
        else
        {
            // shouldn't happen and doesn't make sense, so disregard entry
            return;
        }

    }

    this->updateEstimate();
}

void MiniSync::MiniSyncAlgorithm::addDataPoint(uint64_t t_o, uint64_t t_b, uint64_t t_r)
{

}
