// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/transport.hpp>
#include "tempo.hpp"

using element::Transport;
using element::TransportAction;

namespace {

/** Runs one empty audio cycle so requested state is applied and published. */
void cycle (Transport& transport)
{
    transport.preProcess (0);
    transport.postProcess (0);
}

} // namespace

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

BOOST_AUTO_TEST_CASE (requestActionFollowsButtonRules)
{
    Transport transport;
    auto monitor = transport.getMonitor();

    transport.requestAction (TransportAction::Play);
    cycle (transport);
    BOOST_REQUIRE (monitor->playing.get());

    // Play while playing restarts from the beginning and keeps playing.
    transport.requestAudioFrame (1000);
    cycle (transport);
    BOOST_REQUIRE_EQUAL (transport.getPositionFrames(), 1000);
    transport.requestAction (TransportAction::Play);
    cycle (transport);
    BOOST_REQUIRE (monitor->playing.get());
    BOOST_REQUIRE_EQUAL (transport.getPositionFrames(), 0);

    // Stop while playing stops without moving.
    transport.requestAudioFrame (1000);
    cycle (transport);
    transport.requestAction (TransportAction::Stop);
    cycle (transport);
    BOOST_REQUIRE (! monitor->playing.get());
    BOOST_REQUIRE_EQUAL (transport.getPositionFrames(), 1000);

    // Stop while stopped rewinds.
    transport.requestAction (TransportAction::Stop);
    cycle (transport);
    BOOST_REQUIRE (! monitor->playing.get());
    BOOST_REQUIRE_EQUAL (transport.getPositionFrames(), 0);

    // Record toggles.
    transport.requestAction (TransportAction::Record);
    cycle (transport);
    BOOST_REQUIRE (monitor->recording.get());
    transport.requestAction (TransportAction::Record);
    cycle (transport);
    BOOST_REQUIRE (! monitor->recording.get());

    // Seek zero rewinds regardless of play state.
    transport.requestAudioFrame (500);
    cycle (transport);
    transport.requestAction (TransportAction::SeekZero);
    cycle (transport);
    BOOST_REQUIRE_EQUAL (transport.getPositionFrames(), 0);
}

BOOST_AUTO_TEST_CASE (transportActionStringsRoundTrip)
{
    for (auto action : { TransportAction::Play, TransportAction::Stop, TransportAction::Record, TransportAction::SeekZero }) {
        auto parsed = element::transportActionFromString (element::toString (action));
        BOOST_REQUIRE (parsed.has_value());
        BOOST_REQUIRE (*parsed == action);
        BOOST_REQUIRE (element::getTransportActionName (action).isNotEmpty());
    }
    BOOST_REQUIRE (! element::transportActionFromString ("bogus").has_value());
    BOOST_REQUIRE (! element::transportActionFromString ({}).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
