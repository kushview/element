// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

// Integration test for the Learn result: capturing a raw MIDI event creates a
// flat MidiMapping that is live immediately. Drives the real MappingEngine
// capture signal + real Session storage (the MappingService glue over these
// seams is trivial delegation).

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

BOOST_AUTO_TEST_SUITE (MappingLearnTests)

BOOST_AUTO_TEST_CASE (CaptureCreatesLiveMapping)
{
    Context context;
    auto session = context.session();

    ProcessorPtr obj = new ParamTestNode (2);
    auto nodeId = addLiveNode (session, obj);
    auto param = obj->getParameters()[0];

    MappingEngine engine;
    engine.rebuildBindings (session);
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 0);

    // Phase 1 result: parameter target chosen (node + index 0).
    const int capturedParameter = 0;

    // Phase 2: arm capture and feed a raw MIDI event that matches no mapping.
    engine.mappingCapturedSignal().connect ([&] {
        auto mapping = MidiMapping::fromCapture (
            engine.getCapturedDevice(), engine.getCapturedMessage(), "parameter", nodeId, capturedParameter);
        session->addMidiMapping (mapping);
        engine.rebuildBindings (session);
    });

    engine.captureMapping (true);
    engine.process ("controller-1", MidiMessage::controllerEvent (1, 21, 0));

    // exactly one mapping, targeting our node/param
    BOOST_REQUIRE_EQUAL (session->getNumMidiMappings(), 1);
    auto created = session->getMidiMapping (0);
    BOOST_REQUIRE (created.getNodeUuid() == nodeId);
    BOOST_REQUIRE_EQUAL (created.getParameterIndex(), capturedParameter);
    BOOST_REQUIRE_EQUAL (created.getEventId(), 21);
    BOOST_REQUIRE (created.getDevice() == "controller-1");

    // live immediately: a subsequent matching message drives the parameter
    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);
    engine.process ("controller-1", MidiMessage::controllerEvent (1, 21, 127));
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);
}

BOOST_AUTO_TEST_SUITE_END()
