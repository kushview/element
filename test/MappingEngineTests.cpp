// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/node.hpp>
#include <element/processor.hpp>
#include <element/session.hpp>
#include <element/midimapping.hpp>
#include <element/tags.hpp>

using namespace element;
using namespace juce;

#include "engine/mappingengine.hpp"
#include "fixture/ParamTestNode.h"

namespace {

/** Insert a live node (with a Processor object) into the session's first graph
    and return its uuid. */
static Uuid addLiveNode (SessionPtr session, ProcessorPtr obj)
{
    auto graphs = session->getValueTree().getChildWithName (tags::graphs);
    auto graph = graphs.getChild (0);
    if (! graph.isValid()) {
        graph = ValueTree (types::Node);
        graph.setProperty (tags::uuid, Uuid().toString(), nullptr);
        graph.getOrCreateChildWithName (tags::nodes, nullptr);
        graphs.addChild (graph, -1, nullptr);
    }

    auto nodes = graph.getOrCreateChildWithName (tags::nodes, nullptr);
    Uuid nodeId;
    ValueTree node (types::Node);
    node.setProperty (tags::uuid, nodeId.toString(), nullptr);
    node.setProperty (tags::object, obj.get(), nullptr);
    nodes.addChild (node, -1, nullptr);
    return nodeId;
}

} // namespace

BOOST_AUTO_TEST_SUITE (MappingEngineTests)

BOOST_AUTO_TEST_CASE (RoutesMatchingMessage)
{
    Context context;
    auto session = context.session();

    ProcessorPtr obj = new ParamTestNode (2);
    auto nodeId = addLiveNode (session, obj);
    auto param = obj->getParameters()[0];

    session->addMidiMapping (MidiMapping::fromCapture (
        "dev-A", MidiMessage::controllerEvent (1, 7, 0), "parameter", nodeId, 0));

    MappingEngine engine;
    engine.rebuildBindings (session);

    engine.process ("dev-A", MidiMessage::controllerEvent (1, 7, 127));
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);

    // non-matching CC number does nothing
    engine.process ("dev-A", MidiMessage::controllerEvent (1, 8, 0));
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE (DeviceFilter)
{
    Context context;
    auto session = context.session();

    ProcessorPtr obj = new ParamTestNode (1);
    auto nodeId = addLiveNode (session, obj);
    auto param = obj->getParameters()[0];

    session->addMidiMapping (MidiMapping::fromCapture (
        "dev-A", MidiMessage::controllerEvent (1, 7, 0), "parameter", nodeId, 0));

    MappingEngine engine;
    engine.rebuildBindings (session);

    // wrong device ignored
    engine.process ("dev-B", MidiMessage::controllerEvent (1, 7, 127));
    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);
    // right device routes
    engine.process ("dev-A", MidiMessage::controllerEvent (1, 7, 127));
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE (EmptyDeviceMatchesAny)
{
    Context context;
    auto session = context.session();

    ProcessorPtr obj = new ParamTestNode (1);
    auto nodeId = addLiveNode (session, obj);
    auto param = obj->getParameters()[0];

    auto m = MidiMapping::fromCapture ("", MidiMessage::controllerEvent (1, 7, 0), "parameter", nodeId, 0);
    session->addMidiMapping (m);

    MappingEngine engine;
    engine.rebuildBindings (session);
    engine.process ("anything", MidiMessage::controllerEvent (1, 7, 127));
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE (CaptureDoesNotRoute)
{
    Context context;
    auto session = context.session();

    ProcessorPtr obj = new ParamTestNode (1);
    auto nodeId = addLiveNode (session, obj);
    auto param = obj->getParameters()[0];

    session->addMidiMapping (MidiMapping::fromCapture (
        "dev-A", MidiMessage::controllerEvent (1, 7, 0), "parameter", nodeId, 0));

    MappingEngine engine;
    engine.rebuildBindings (session);

    int fired = 0;
    auto conn = engine.mappingCapturedSignal().connect ([&fired] { ++fired; });

    engine.captureMapping (true);
    BOOST_REQUIRE (engine.isCapturingMapping());

    // an event that matches NO existing mapping must still be captured (the fix)
    engine.process ("dev-Z", MidiMessage::controllerEvent (4, 99, 55));

    BOOST_REQUIRE_EQUAL (fired, 1);
    BOOST_REQUIRE (! engine.isCapturingMapping());
    BOOST_REQUIRE (engine.getCapturedDevice() == "dev-Z");
    BOOST_REQUIRE (engine.getCapturedMessage().isController());
    BOOST_REQUIRE_EQUAL (engine.getCapturedMessage().getControllerNumber(), 99);
    // routing did not occur during capture
    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);

    conn.disconnect();
}

BOOST_AUTO_TEST_SUITE_END()
