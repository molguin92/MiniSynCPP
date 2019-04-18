/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "stats.h"
#include <fstream>
#include <loguru/loguru.hpp>

void MiniSync::Stats::SyncStats::add_sample(int64_t offset, double offset_error, double drift, double drift_error)
{
    Sample n_sample;
    n_sample.offset = offset;
    n_sample.offset_error = offset_error;
    n_sample.drift = drift;
    n_sample.drift_error = drift_error;

    this->samples.push_back(n_sample);
}

uint32_t MiniSync::Stats::SyncStats::write_csv(const std::string& path)
{
    LOG_F(INFO, "Writing CSV file with synchronization stats.");
    int i = 0;
    try
    {
        std::ofstream outfile{path, std::ofstream::out};
        // write header
        outfile << "Sample;Drift;Drift Error;Offset;Offset Error" << std::endl;
        for (; i < this->samples.size(); i++)
        {
            const Sample& s = this->samples.at(i);
            outfile << i << ";"
                    << s.drift << ";" << s.drift_error << ";"
                    << s.offset << ";" << s.offset_error << std::endl;
        }
        // done
        outfile.close();
    }
    catch (...)
    {
        // in case of any error, (try to) remove the file
        LOG_F(ERROR, "Writing to file failed!");
        remove(path.c_str());
        __throw_exception_again;
    }

    return i - 1; // number of written records
}
