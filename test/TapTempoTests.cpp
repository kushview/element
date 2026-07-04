// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/taptempo.hpp>

using namespace element;

BOOST_AUTO_TEST_SUITE (TapTempoTests)

BOOST_AUTO_TEST_CASE (FirstTapReturnsNothing)
{
    TapTempo tap;
    BOOST_REQUIRE (! tap.tap (0.0).has_value());
}

BOOST_AUTO_TEST_CASE (SteadyTapsYieldTempo)
{
    // 500 ms between taps == 120 BPM.
    TapTempo tap;
    BOOST_REQUIRE (! tap.tap (0.0).has_value());
    auto a = tap.tap (500.0);
    BOOST_REQUIRE (a.has_value());
    BOOST_CHECK_CLOSE (*a, 120.0, 0.0001);
    auto b = tap.tap (1000.0);
    BOOST_REQUIRE (b.has_value());
    BOOST_CHECK_CLOSE (*b, 120.0, 0.0001);
    auto c = tap.tap (1500.0);
    BOOST_REQUIRE (c.has_value());
    BOOST_CHECK_CLOSE (*c, 120.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (SteadyTapsPastResetIntervalDoNotFalselyReset)
{
    // Regression: the reset must be measured from the *previous* tap, not the
    // first, so a steady run longer than the reset interval keeps averaging.
    TapTempo tap; // default reset interval 2000 ms
    BOOST_REQUIRE (! tap.tap (0.0).has_value());
    double last = 0.0;
    for (int i = 1; i <= 8; ++i) // 4000 ms total at 500 ms spacing
    {
        auto r = tap.tap (i * 500.0);
        BOOST_REQUIRE (r.has_value());
        last = *r;
    }
    BOOST_CHECK_CLOSE (last, 120.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (GapResetsTheRun)
{
    TapTempo tap;
    tap.setResetInterval (2000.0);
    BOOST_REQUIRE (! tap.tap (0.0).has_value());
    BOOST_REQUIRE (tap.tap (500.0).has_value());
    // A gap larger than the reset interval starts a fresh run: nothing yet.
    BOOST_REQUIRE (! tap.tap (3000.0).has_value());
    auto r = tap.tap (3250.0); // 250 ms == 240 BPM
    BOOST_REQUIRE (r.has_value());
    BOOST_CHECK_CLOSE (*r, 240.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (ClampsToRange)
{
    TapTempo tap;
    tap.setRange (20.0, 999.0);
    BOOST_REQUIRE (! tap.tap (0.0).has_value());
    // 10 ms == 6000 BPM, clamped to 999.
    auto r = tap.tap (10.0);
    BOOST_REQUIRE (r.has_value());
    BOOST_CHECK_CLOSE (*r, 999.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (ResetClearsRun)
{
    TapTempo tap;
    BOOST_REQUIRE (! tap.tap (0.0).has_value());
    tap.reset();
    // After reset the next tap is again the first of a run.
    BOOST_REQUIRE (! tap.tap (500.0).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
