//
// Created by molguin on 2019-03-29.
//

#ifndef TINYSYNC___MINISYNC_H
#define TINYSYNC___MINISYNC_H

# include <ctime>

namespace MiniSync {
    struct DataPoint {
    public:
        time_t t_o;
        time_t t_br;
        time_t t_bt;
        time_t t_r;
    };

    class SyncAlgorithm
    {
    protected:
        float currentDrift;
        float currentOffset;
        SyncAlgorithm();
    public:
        virtual void addDataPoint(time_t t_o, time_t t_br, time_t t_bt, time_t t_r) = 0;
        float getDrift();
        float getOffset();
        time_t getCurrentTime();
    };

    class TinySyncAlgorithm: public SyncAlgorithm
    {
    };

    class MiniSyncAlgorithm: public SyncAlgorithm
    {};
}


#endif //TINYSYNC___MINISYNC_H
