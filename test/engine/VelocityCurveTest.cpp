// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include "engine/velocitycurve.hpp"

using namespace element;

class VelocityCurveUnit {
public:
    VelocityCurveUnit() {}
    void runTest()
    {
        VelocityCurve curve;
        BOOST_REQUIRE (curve.getMode() == VelocityCurve::Linear);
        BOOST_REQUIRE (0.6f == curve.process (0.6f));

        curve.setMode (VelocityCurve::Soft_1);
        BOOST_REQUIRE (0.f == std::floor (127.f * curve.process (0.f)));
        BOOST_REQUIRE (90.0 == std::floor (127.f * curve.process (100.f / 127.f)));
        BOOST_REQUIRE (38.0 == std::floor (127.f * curve.process (50.f / 127.f)));
        BOOST_REQUIRE (17.0 == std::floor (127.f * curve.process (25.f / 127.f)));
        BOOST_REQUIRE (127.f == std::floor (127.f * curve.process (1.f)));

        curve.setMode (VelocityCurve::Hard_1);
        BOOST_REQUIRE (0.f == std::floor (127.f * curve.process (0.f)));
        BOOST_REQUIRE (107.0 == std::floor (127.f * curve.process (100.f / 127.f)));
        BOOST_REQUIRE (62.0 == std::floor (127.f * curve.process (50.f / 127.f)));
        BOOST_REQUIRE (33.0 == std::floor (127.f * curve.process (25.f / 127.f)));
        BOOST_REQUIRE (127.f == std::floor (127.f * curve.process (1.f)));
    }
};

BOOST_AUTO_TEST_SUITE (VelocityCurveTest)

BOOST_AUTO_TEST_CASE (Basics)
{
    VelocityCurveUnit().runTest();
}

BOOST_AUTO_TEST_SUITE_END()
