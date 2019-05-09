/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <minisync_api.h>

TEST_CASE("Basic testing of TinySync and MiniSync", "[TinySync, MiniSync]")
{

    std::shared_ptr<MiniSync::API::Algorithm> algorithm;
    REQUIRE(algorithm == nullptr);

    SECTION("Set up TinySync")
    {
        algorithm = MiniSync::API::Factory::createTinySync();
    }
    SECTION("Set up MiniSync")
    {
        algorithm = MiniSync::API::Factory::createMiniSync();
    }

    REQUIRE(algorithm != nullptr); // check that the algorithm is not a nullpointer
    REQUIRE(algorithm->getDrift() == 1.0);
    REQUIRE(algorithm->getDriftError() == 0.0);
    REQUIRE(algorithm->getOffset() == MiniSync::us_t{0});
    REQUIRE(algorithm->getOffsetError() == MiniSync::us_t{0});

    // initial conditions
    MiniSync::us_t To{-1}, Tbr{0}, Tbt{1}, Tr{2};

    // initial offset and drift
    // initial coordinates are on x = 0, so max_offset and min_offset should simply be the y coordinates
    auto high_drift = (To - Tr) / (Tbt - Tbr);
    auto low_drift = (Tr - To) / (Tbt - Tbr);
    auto init_drift = (high_drift + low_drift) / 2.0;
    auto init_drift_error = (low_drift - high_drift) / 2.0;

    auto init_offset = (To + Tr) / 2.0;
    auto init_offset_error = (Tr - To) / 2.0;

    algorithm->addDataPoint(To, Tbr, Tr);
    // just adding one point does not trigger an update
    REQUIRE(algorithm->getDrift() == 1.0);
    REQUIRE(algorithm->getDriftError() == 0.0);
    REQUIRE(algorithm->getOffset() == MiniSync::us_t{0});
    REQUIRE(algorithm->getOffsetError() == MiniSync::us_t{0});

    algorithm->addDataPoint(To, Tbt, Tr);
    // now algorithm should recalculate
    REQUIRE(algorithm->getDrift() == init_drift);
    REQUIRE(algorithm->getDriftError() == init_drift_error);
    REQUIRE(algorithm->getOffset() == init_offset);
    REQUIRE(algorithm->getOffsetError() == init_offset_error);
}
