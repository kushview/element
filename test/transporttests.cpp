// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/transport.hpp>
#include "tempo.hpp"

using element::Transport;

BOOST_AUTO_TEST_SUITE (TransportTests)

BOOST_AUTO_TEST_CASE (publishesCorrectTimeSignatureDenominator)
{
    Transport transport;

    auto pos = transport.getPosition();
    BOOST_REQUIRE (pos.hasValue());
    BOOST_REQUIRE (pos->getTimeSignature().hasValue());
    BOOST_REQUIRE_EQUAL (pos->getTimeSignature()->numerator, 4);
    BOOST_REQUIRE_EQUAL (pos->getTimeSignature()->denominator, 4);

    transport.requestMeter (7, element::BeatType::EighthNote);
    transport.postProcess (0);

    pos = transport.getPosition();
    BOOST_REQUIRE (pos.hasValue());
    BOOST_REQUIRE (pos->getTimeSignature().hasValue());
    BOOST_REQUIRE_EQUAL (pos->getTimeSignature()->numerator, 7);
    BOOST_REQUIRE_EQUAL (pos->getTimeSignature()->denominator, 8);
}

BOOST_AUTO_TEST_SUITE_END()
