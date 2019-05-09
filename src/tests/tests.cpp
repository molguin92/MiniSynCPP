/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <minisync_api.h>

TEST_CASE("Testing TinySync", "[TinySync]")
{

    auto TinySync = MiniSync::API::Factory::createTinySync();

    // initial conditions
    MiniSync::us_t To{0}, Tbr{1}, Tbt{2}, Tr{3};
    TinySync->addDataPoint(To, Tbr, Tr);

    REQUIRE(TinySync->getDrift() == 1.0);
    REQUIRE(TinySync->getDriftError() == 0.0);
    REQUIRE(TinySync->getOffset() == MiniSync::us_t{0});
    REQUIRE(TinySync->getOffsetError() == MiniSync::us_t{0});

    TinySync->addDataPoint(To, Tbt, Tr);

    REQUIRE(TinySync->getDrift() == 0.0);
    REQUIRE(TinySync->getOffset() == MiniSync::us_t{1.5});

}
