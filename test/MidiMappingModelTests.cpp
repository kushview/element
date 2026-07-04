// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/midimapping.hpp>

using namespace element;
using namespace juce;

BOOST_AUTO_TEST_SUITE (MidiMappingModelTests)

BOOST_AUTO_TEST_CASE (DefaultsStabilize)
{
    MidiMapping m ("Test");
    BOOST_REQUIRE (m.isValid());
    BOOST_REQUIRE (Uuid (m.getUuidString()).toString() == m.getUuidString());
    BOOST_REQUIRE (m.getName() == "Test");
    BOOST_REQUIRE (m.getDevice().isEmpty());
    BOOST_REQUIRE (m.getEventType() == "controller");
    BOOST_REQUIRE_EQUAL (m.getEventId(), 0);
    BOOST_REQUIRE_EQUAL (m.getMidiChannel(), 0);
    BOOST_REQUIRE (! m.isToggle());
    BOOST_REQUIRE (m.getTargetType() == "parameter");
    BOOST_REQUIRE_EQUAL (m.getParameterIndex(), -1);
}

BOOST_AUTO_TEST_CASE (FromCaptureController)
{
    Uuid node;
    auto m = MidiMapping::fromCapture ("dev-1", MidiMessage::controllerEvent (3, 7, 100), "parameter", node, 4);
    BOOST_REQUIRE (m.isControllerEvent());
    BOOST_REQUIRE (! m.isNoteEvent());
    BOOST_REQUIRE_EQUAL (m.getEventId(), 7);
    BOOST_REQUIRE (m.getDevice() == "dev-1");
    BOOST_REQUIRE (m.getNodeUuid() == node);
    BOOST_REQUIRE_EQUAL (m.getParameterIndex(), 4);
}

BOOST_AUTO_TEST_CASE (FromCaptureNote)
{
    Uuid node;
    auto m = MidiMapping::fromCapture ("dev-1", MidiMessage::noteOn (2, 60, (uint8) 90), "parameter", node, 0);
    BOOST_REQUIRE (m.isNoteEvent());
    BOOST_REQUIRE (! m.isControllerEvent());
    BOOST_REQUIRE_EQUAL (m.getEventId(), 60);
}

BOOST_AUTO_TEST_CASE (MatchesController)
{
    Uuid node;
    auto m = MidiMapping::fromCapture ("d", MidiMessage::controllerEvent (1, 7, 0), "parameter", node, 0);
    // omni channel (default 0) matches any channel
    BOOST_REQUIRE (m.matches (MidiMessage::controllerEvent (1, 7, 64)));
    BOOST_REQUIRE (m.matches (MidiMessage::controllerEvent (9, 7, 64)));
    // wrong CC number rejected
    BOOST_REQUIRE (! m.matches (MidiMessage::controllerEvent (1, 8, 64)));
    // note message rejected for a controller mapping
    BOOST_REQUIRE (! m.matches (MidiMessage::noteOn (1, 7, (uint8) 64)));
}

BOOST_AUTO_TEST_CASE (MatchesSpecificChannel)
{
    auto m = MidiMapping::fromCapture ("d", MidiMessage::controllerEvent (1, 7, 0), "parameter", Uuid(), 0);
    m.setProperty (tags::midiChannel, 5);
    BOOST_REQUIRE (m.matches (MidiMessage::controllerEvent (5, 7, 64)));
    BOOST_REQUIRE (! m.matches (MidiMessage::controllerEvent (6, 7, 64)));
}

BOOST_AUTO_TEST_CASE (MatchesNote)
{
    auto m = MidiMapping::fromCapture ("d", MidiMessage::noteOn (1, 60, (uint8) 64), "parameter", Uuid(), 0);
    BOOST_REQUIRE (m.matches (MidiMessage::noteOn (1, 60, (uint8) 100)));
    BOOST_REQUIRE (m.matches (MidiMessage::noteOff (1, 60)));
    BOOST_REQUIRE (! m.matches (MidiMessage::noteOn (1, 61, (uint8) 100)));
    BOOST_REQUIRE (! m.matches (MidiMessage::controllerEvent (1, 60, 64)));
}

BOOST_AUTO_TEST_CASE (ValueTreeRoundTrip)
{
    Uuid node;
    auto m = MidiMapping::fromCapture ("dev-x", MidiMessage::controllerEvent (1, 20, 0), "parameter", node, 2);
    m.setProperty (tags::name, "Filter");

    auto xml = m.data().toXmlString();
    auto reparsed = ValueTree::fromXml (xml);
    BOOST_REQUIRE (reparsed.isValid());

    MidiMapping loaded (reparsed);
    BOOST_REQUIRE (loaded.isValid());
    BOOST_REQUIRE (loaded.getName() == "Filter");
    BOOST_REQUIRE (loaded.getDevice() == "dev-x");
    BOOST_REQUIRE_EQUAL (loaded.getEventId(), 20);
    BOOST_REQUIRE (loaded.getNodeUuid() == node);
    BOOST_REQUIRE_EQUAL (loaded.getParameterIndex(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
