// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/session.hpp>
#include <element/midimapping.hpp>
#include <element/tags.hpp>

using namespace element;
using namespace juce;

BOOST_AUTO_TEST_SUITE (MidiMappingSessionTests)

BOOST_AUTO_TEST_CASE (AddRemoveFind)
{
    Context context;
    auto session = context.session();
    BOOST_REQUIRE (session != nullptr);
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 0);

    auto m = MidiMapping::fromCapture ("dev", MidiMessage::controllerEvent (1, 7, 0), "parameter", Uuid(), 1);
    session->addMidiMapping (m);
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 1);

    auto found = session->findMidiMappingById (Uuid (m.getUuidString()));
    BOOST_REQUIRE (found.isValid());
    BOOST_REQUIRE (found.getUuidString() == m.getUuidString());

    session->removeMidiMapping (m);
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 0);
}

BOOST_AUTO_TEST_CASE (SerializationRoundTrip)
{
    Context context;
    auto session = context.session();

    session->addMidiMapping (MidiMapping::fromCapture ("d1", MidiMessage::controllerEvent (1, 7, 0), "parameter", Uuid(), 0));
    session->addMidiMapping (MidiMapping::fromCapture ("d2", MidiMessage::noteOn (1, 60, (uint8) 1), "parameter", Uuid(), 1));
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 2);

    auto xml = session->getValueTree().toXmlString();
    auto reparsed = ValueTree::fromXml (xml);
    BOOST_REQUIRE (reparsed.isValid());

    auto mappings = reparsed.getChildWithName (tags::midiMappings);
    BOOST_REQUIRE_EQUAL (mappings.getNumChildren(), 2);
}

BOOST_AUTO_TEST_CASE (MigrateLegacyControllerMaps)
{
    // Hand-build a legacy session tree: one controller, one control, one map.
    ValueTree session (types::Session);
    auto controllers = session.getOrCreateChildWithName (tags::controllers, nullptr);
    auto maps = session.getOrCreateChildWithName (tags::maps, nullptr);

    Uuid controllerId, controlId, nodeId;

    ValueTree controller (types::Controller);
    controller.setProperty (tags::uuid, controllerId.toString(), nullptr);
    controller.setProperty (tags::inputDevice, "my-device", nullptr);

    ValueTree control (types::Control);
    control.setProperty (tags::uuid, controlId.toString(), nullptr);
    control.setProperty (tags::name, "Knob 1", nullptr);
    control.setProperty ("eventType", "controller", nullptr);
    control.setProperty ("eventId", 20, nullptr);
    control.setProperty (tags::midiChannel, 3, nullptr);
    control.setProperty ("momentary", false, nullptr);
    controller.addChild (control, -1, nullptr);
    controllers.addChild (controller, -1, nullptr);

    ValueTree map (types::ControllerMap);
    map.setProperty (tags::controller, controllerId.toString(), nullptr);
    map.setProperty (tags::control, controlId.toString(), nullptr);
    map.setProperty (tags::node, nodeId.toString(), nullptr);
    map.setProperty (tags::parameter, 5, nullptr);
    maps.addChild (map, -1, nullptr);

    migrateControllerMaps (session);

    auto mappings = session.getChildWithName (tags::midiMappings);
    BOOST_REQUIRE (mappings.isValid());
    BOOST_REQUIRE_EQUAL (mappings.getNumChildren(), 1);

    MidiMapping m (mappings.getChild (0));
    BOOST_REQUIRE (m.getDevice() == "my-device");
    BOOST_REQUIRE (m.isControllerEvent());
    BOOST_REQUIRE_EQUAL (m.getEventId(), 20);
    BOOST_REQUIRE_EQUAL (m.getMidiChannel(), 3);
    BOOST_REQUIRE (m.isToggle()); // momentary == false => toggle == true
    BOOST_REQUIRE (m.getNodeUuid() == nodeId);
    BOOST_REQUIRE_EQUAL (m.getParameterIndex(), 5);
}

BOOST_AUTO_TEST_CASE (CleanOrphans)
{
    Context context;
    auto session = context.session();

    // Add a graph node with a known uuid so findNodeById can resolve it.
    Uuid liveNode;
    auto graphs = session->getValueTree().getChildWithName (tags::graphs);
    ValueTree graph (types::Node);
    graph.setProperty (tags::uuid, liveNode.toString(), nullptr);
    graphs.addChild (graph, -1, nullptr);

    session->addMidiMapping (MidiMapping::fromCapture ("d", MidiMessage::controllerEvent (1, 7, 0), "parameter", liveNode, 0));
    session->addMidiMapping (MidiMapping::fromCapture ("d", MidiMessage::controllerEvent (1, 8, 0), "parameter", Uuid(), 0));
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 2);

    session->cleanOrphanMidiMappings();
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 1);
    BOOST_REQUIRE (session->getMidiMapping (0).getNodeUuid() == liveNode);
}

BOOST_AUTO_TEST_SUITE_END()
