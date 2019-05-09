/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/
#include <minisync_api.h>
#include <catch2/catch.hpp>
#include <sstream>
#include <thread> // sleep_for
#include <random>

namespace Catch
{
    template<>
    struct StringMaker<MiniSync::us_t>
    {
        static std::string convert(const MiniSync::us_t& value)
        {
            auto ns = std::chrono::duration_cast<std::chrono::duration<long long int, std::nano>>(value);
            auto ms = (ns.count() / 1000) + ((ns.count() % 1000) / 1000.0);

            std::ostringstream os;
            os << ms << " µs";
            return os.str();
        }
    };
}

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
    REQUIRE(algorithm->getDrift() == Approx(1.0).epsilon(0.001));
    REQUIRE(algorithm->getDriftError() == Approx(0.0).epsilon(0.001));
    REQUIRE(algorithm->getOffset().count() == Approx(MiniSync::us_t{0}.count()).epsilon(0.001));
    REQUIRE(algorithm->getOffsetError().count() == Approx(MiniSync::us_t{0}.count()).epsilon(0.001));

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
    REQUIRE(algorithm->getDrift() == Approx(1.0).epsilon(0.001));
    REQUIRE(algorithm->getDriftError() == Approx(0.0).epsilon(0.001));
    REQUIRE(algorithm->getOffset().count() == Approx(MiniSync::us_t{0}.count()).epsilon(0.001));
    REQUIRE(algorithm->getOffsetError().count() == Approx(MiniSync::us_t{0}.count()).epsilon(0.001));

    algorithm->addDataPoint(To, Tbt, Tr);
    // now algorithm should recalculate
    REQUIRE(algorithm->getDrift() == Approx(init_drift).epsilon(0.001));
    REQUIRE(algorithm->getDriftError() == Approx(init_drift_error).epsilon(0.001));
    REQUIRE(algorithm->getOffset().count() == Approx(init_offset.count()).epsilon(0.001));
    REQUIRE(algorithm->getOffsetError().count() == Approx(init_offset_error.count()).epsilon(0.001));
}

TEST_CASE("Side by side tests", "[timing]")
{
    auto tiny = MiniSync::API::Factory::createTinySync();
    auto mini = MiniSync::API::Factory::createMiniSync();

    auto T0 = std::chrono::steady_clock::now();
    using clock = std::chrono::steady_clock;

    // we'll use some random delays, that way we get some variability in the inputs to the algorithms
    std::default_random_engine gen;
    std::uniform_int_distribution<uint32_t> dist(0, 10);

    // run algorithms in a loop. MiniSync should always give an equal OR better result compared to TinySync
    for (int i = 0; i < 50; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{dist(gen)});
        
        auto To = clock::now() - T0;
        std::this_thread::sleep_for(std::chrono::milliseconds{dist(gen)});
        auto Tb = clock::now() - T0;
        std::this_thread::sleep_for(std::chrono::milliseconds{dist(gen)});
        auto Tr = clock::now() - T0;

        tiny->addDataPoint(To, Tb, Tr);
        mini->addDataPoint(To, Tb, Tr);

        REQUIRE(mini->getOffsetError() <= tiny->getOffsetError());
        REQUIRE(mini->getDriftError() <= tiny->getDriftError());
    }
}
