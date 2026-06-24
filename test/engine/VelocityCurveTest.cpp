// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <juce_audio_basics/juce_audio_basics.h>

#include "engine/velocitycurve.hpp"

using namespace element;

BOOST_AUTO_TEST_SUITE (VelocityCurveTest)

BOOST_AUTO_TEST_CASE (Basics)
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

BOOST_AUTO_TEST_CASE (LinearIdentity)
{
    VelocityCurve curve;
    BOOST_REQUIRE (curve.getMode() == VelocityCurve::Linear);

    for (uint8_t v = 0; v < 128; ++v)
    {
        float fv = static_cast<float> (v) / 127.f;
        BOOST_REQUIRE_EQUAL (curve.process (fv), fv);
        BOOST_REQUIRE_EQUAL (curve.process (v), v);
    }
}

BOOST_AUTO_TEST_CASE (Uint8Process)
{
    const VelocityCurve::Mode modes[] = {
        VelocityCurve::Soft_1, VelocityCurve::Soft_2, VelocityCurve::Soft_3,
        VelocityCurve::Hard_1, VelocityCurve::Hard_2, VelocityCurve::Hard_3
    };

    for (auto mode : modes)
    {
        VelocityCurve curve;
        curve.setMode (mode);

        for (int v = 0; v <= 127; ++v)
        {
            auto u8result = curve.process (static_cast<uint8_t> (v));
            float floatResult = curve.process (static_cast<float> (v) / 127.f);
            auto expected = static_cast<uint8_t> (juce::roundToIntAccurate (127.f * floatResult));

            BOOST_CHECK_EQUAL (u8result, expected);
            BOOST_CHECK_LE (u8result, 127);
        }
    }

    VelocityCurve maxCurve;
    maxCurve.setMode (VelocityCurve::Max);
    for (int v = 0; v <= 127; ++v)
        BOOST_CHECK_EQUAL (maxCurve.process (static_cast<uint8_t> (v)), 127);
}

BOOST_AUTO_TEST_CASE (FullRangeRoundTrip)
{
    const VelocityCurve::Mode modes[] = {
        VelocityCurve::Linear,
        VelocityCurve::Soft_1, VelocityCurve::Soft_2, VelocityCurve::Soft_3,
        VelocityCurve::Hard_1, VelocityCurve::Hard_2, VelocityCurve::Hard_3,
        VelocityCurve::Max
    };

    for (auto mode : modes)
    {
        VelocityCurve curve;
        curve.setMode (mode);

        for (int v = 0; v <= 127; ++v)
        {
            float fv = static_cast<float> (v) / 127.f;
            float processed = curve.process (fv);
            int result = juce::roundToInt (processed * 127.f);

            BOOST_CHECK_GE (result, 0);
            BOOST_CHECK_LE (result, 127);
        }
    }
}

BOOST_AUTO_TEST_CASE (MidiMessageRoundTrip)
{
    VelocityCurve curve;

    for (int v = 1; v <= 127; ++v)
    {
        auto msg = juce::MidiMessage::noteOn (1, 60, (juce::uint8) v);
        BOOST_REQUIRE_EQUAL (msg.getVelocity(), v);

        msg.setVelocity (curve.process (msg.getFloatVelocity()));
        BOOST_CHECK_EQUAL ((int) msg.getVelocity(), v);
    }
}

BOOST_AUTO_TEST_SUITE_END()
