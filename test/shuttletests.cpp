// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/shuttle.hpp>

using element::Shuttle;

BOOST_AUTO_TEST_SUITE (ShuttleTests)

BOOST_AUTO_TEST_CASE (defaults)
{
    Shuttle stl;
    BOOST_REQUIRE_EQUAL (stl.getSampleRate(), 44100.0);
    BOOST_REQUIRE_EQUAL (stl.getTempo(), 120.f);
    BOOST_REQUIRE_EQUAL (stl.getFramesPerBeat(), 44100.0 * 60.0 / stl.getTempo());
}

BOOST_AUTO_TEST_SUITE_END()
