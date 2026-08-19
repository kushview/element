// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include "nodes/midimonitor.hpp"
#include "utils.hpp"

using namespace element;
using namespace juce;

BOOST_AUTO_TEST_SUITE (MidiMonitorTests)

BOOST_AUTO_TEST_CASE (NoteOnUsesMiddleCAsC4)
{
    const auto text = MidiMonitorNode::describe (MidiMessage::noteOn (1, 60, 1.0f));
    BOOST_REQUIRE (text.startsWith ("Note On C4"));
    BOOST_REQUIRE (text.contains ("(60)"));
    BOOST_REQUIRE (text.contains ("Channel 1"));
}

BOOST_AUTO_TEST_CASE (NoteNamesAtOctaveEdges)
{
    BOOST_REQUIRE (MidiMonitorNode::describe (MidiMessage::noteOn (1, 21, 1.0f))
                       .startsWith ("Note On A0"));
    BOOST_REQUIRE (MidiMonitorNode::describe (MidiMessage::noteOn (1, 0, 1.0f))
                       .startsWith ("Note On C-1"));
    BOOST_REQUIRE (MidiMonitorNode::describe (MidiMessage::noteOn (1, 127, 1.0f))
                       .startsWith ("Note On G9"));
}

BOOST_AUTO_TEST_CASE (NoteOff)
{
    const auto text = MidiMonitorNode::describe (MidiMessage::noteOff (2, 60));
    BOOST_REQUIRE (text.startsWith ("Note Off C4"));
    BOOST_REQUIRE (text.contains ("Channel 2"));
}

BOOST_AUTO_TEST_CASE (ClockIsNotLogged)
{
    BOOST_REQUIRE (MidiMonitorNode::describe (MidiMessage::midiClock()).isEmpty());
}

BOOST_AUTO_TEST_CASE (TransportMessagesLogOnce)
{
    BOOST_REQUIRE_EQUAL (MidiMonitorNode::describe (MidiMessage::midiStart()).toStdString(), "Start");
    BOOST_REQUIRE_EQUAL (MidiMonitorNode::describe (MidiMessage::midiStop()).toStdString(), "Stop");
    BOOST_REQUIRE_EQUAL (MidiMonitorNode::describe (MidiMessage::midiContinue()).toStdString(), "Continue");
}

BOOST_AUTO_TEST_CASE (AgreesWithNoteValueConversions)
{
    BOOST_REQUIRE_EQUAL (Util::middleCOctave, 4);
    BOOST_REQUIRE_EQUAL (Util::noteValueToString (60).toStdString(), "C4");
    BOOST_REQUIRE_EQUAL (Util::noteValueToString (21).toStdString(), "A0");
    BOOST_REQUIRE_EQUAL (Util::noteValueFromString ("C4"), 60);
    BOOST_REQUIRE_EQUAL (Util::noteValueFromString ("A0"), 21);
    BOOST_REQUIRE_EQUAL (Util::noteValueFromString ("C-1"), 0);
    BOOST_REQUIRE_EQUAL (Util::noteValueFromString ("G9"), 127);
}

BOOST_AUTO_TEST_SUITE_END()
