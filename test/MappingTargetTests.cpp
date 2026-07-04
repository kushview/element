// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/node.hpp>
#include <element/processor.hpp>
#include <element/tags.hpp>

using namespace element;
using namespace juce;

#include "engine/mappingtarget.hpp"
#include "fixture/ParamTestNode.h"

namespace {

/** Wrap a live Processor in a Node model (as the graph manager does). */
static Node makeNode (ProcessorPtr obj)
{
    ValueTree data (types::Node);
    data.setProperty (tags::uuid, Uuid().toString(), nullptr);
    data.setProperty (tags::object, obj.get(), nullptr);
    return Node (data, false);
}

} // namespace

BOOST_AUTO_TEST_SUITE (MappingTargetTests)

BOOST_AUTO_TEST_CASE (CCToParameter)
{
    ProcessorPtr obj = new ParamTestNode (2);
    auto node = makeNode (obj);
    ParameterTarget target (node, 0);
    BOOST_REQUIRE (target.isValid());

    auto param = obj->getParameters()[0];

    target.apply (MidiMessage::controllerEvent (1, 7, 127), false);
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);

    target.apply (MidiMessage::controllerEvent (1, 7, 0), false);
    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);

    target.apply (MidiMessage::controllerEvent (1, 7, 64), false);
    BOOST_REQUIRE_CLOSE (param->getValue(), 64.0f / 127.0f, 0.5f);
}

BOOST_AUTO_TEST_CASE (NoteMomentary)
{
    ProcessorPtr obj = new ParamTestNode (1);
    auto node = makeNode (obj);
    ParameterTarget target (node, 0);
    auto param = obj->getParameters()[0];

    target.apply (MidiMessage::noteOn (1, 60, (uint8) 100), false);
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);

    target.apply (MidiMessage::noteOff (1, 60), false);
    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);
}

BOOST_AUTO_TEST_CASE (NoteToggle)
{
    ProcessorPtr obj = new ParamTestNode (1);
    auto node = makeNode (obj);
    ParameterTarget target (node, 0);
    auto param = obj->getParameters()[0];

    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);
    target.apply (MidiMessage::noteOn (1, 60, (uint8) 100), true);
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);
    // note-off ignored in toggle mode
    target.apply (MidiMessage::noteOff (1, 60), true);
    BOOST_REQUIRE_CLOSE (param->getValue(), 1.0f, 0.01f);
    // next note-on flips back to 0
    target.apply (MidiMessage::noteOn (1, 60, (uint8) 100), true);
    BOOST_REQUIRE_SMALL (param->getValue(), 0.01f);
}

BOOST_AUTO_TEST_CASE (SpecialMuteParameter)
{
    // Mute lives on the Node model and does not depend on a parent graph,
    // so it cleanly exercises the special-parameter branch.
    ProcessorPtr obj = new ParamTestNode (1);
    auto node = makeNode (obj);
    ParameterTarget target (node, Processor::MuteParameter);
    BOOST_REQUIRE (target.isValid());

    BOOST_REQUIRE (! node.isMuted());
    target.apply (MidiMessage::controllerEvent (1, 7, 127), false); // >=64 => on
    BOOST_REQUIRE (node.isMuted());
    target.apply (MidiMessage::controllerEvent (1, 7, 0), false); // <64 => off
    BOOST_REQUIRE (! node.isMuted());

    // note toggle flips on each note-on
    target.apply (MidiMessage::noteOn (1, 60, (uint8) 100), true);
    BOOST_REQUIRE (node.isMuted());
    target.apply (MidiMessage::noteOn (1, 60, (uint8) 100), true);
    BOOST_REQUIRE (! node.isMuted());
}

BOOST_AUTO_TEST_CASE (CCToOutputGain)
{
    // dB range mirrors ParameterTarget::applyGain: [-60, +6] dB.
    ProcessorPtr obj = new ParamTestNode (1);
    auto node = makeNode (obj);
    ParameterTarget target (node, Processor::OutputGainParameter);
    BOOST_REQUIRE (target.isValid());

    target.apply (MidiMessage::controllerEvent (1, 7, 127), false); // +6 dB
    BOOST_REQUIRE_CLOSE (obj->getGain(), (float) Decibels::decibelsToGain (6.0), 0.5f);
    BOOST_REQUIRE_CLOSE ((float) node.getProperty (tags::gain), obj->getGain(), 0.01f);

    target.apply (MidiMessage::controllerEvent (1, 7, 0), false); // floor => silence
    BOOST_REQUIRE_SMALL (obj->getGain(), 0.0001f);
}

BOOST_AUTO_TEST_CASE (CCToInputGain)
{
    ProcessorPtr obj = new ParamTestNode (1);
    auto node = makeNode (obj);
    ParameterTarget target (node, Processor::InputGainParameter);
    BOOST_REQUIRE (target.isValid());

    target.apply (MidiMessage::controllerEvent (1, 7, 127), false);
    BOOST_REQUIRE_CLOSE (obj->getInputGain(), (float) Decibels::decibelsToGain (6.0), 0.5f);
    BOOST_REQUIRE_CLOSE ((float) node.getProperty (tags::inputGain), obj->getInputGain(), 0.01f);
}

BOOST_AUTO_TEST_CASE (TempoTargetTapsFromNoteOn)
{
    // A tempo target writes an in-range BPM to the session tempo property on the
    // second (and later) note-on; note-off and CC are ignored. Exact timing is
    // covered by TapTempoTests; here we assert the adapter's routing + clamping.
    ValueTree session (types::Session);
    session.setProperty (tags::tempo, 100.0, nullptr);
    TapTempo shared;
    TempoTarget target (session, shared);
    BOOST_REQUIRE (target.isValid());

    // Taps are timed by the message timestamp (arrival time in ms), which the
    // router stamps in production. 500 ms apart == 120 BPM.
    auto noteAt = [] (double ms) {
        auto m = MidiMessage::noteOn (1, 60, (uint8) 100);
        m.setTimeStamp (ms);
        return m;
    };

    // First note-on seeds the run; tempo unchanged.
    target.apply (noteAt (0.0), false);
    BOOST_REQUIRE_CLOSE ((double) session.getProperty (tags::tempo), 100.0, 0.0001);

    // Second note-on 500 ms later writes exactly 120 BPM.
    target.apply (noteAt (500.0), false);
    BOOST_REQUIRE_CLOSE ((double) session.getProperty (tags::tempo), 120.0, 0.0001);

    // Note-off and CC never tap: tempo stays put.
    session.setProperty (tags::tempo, 100.0, nullptr);
    target.apply (MidiMessage::noteOff (1, 60), false);
    target.apply (MidiMessage::controllerEvent (1, 7, 127), false);
    BOOST_REQUIRE_CLOSE ((double) session.getProperty (tags::tempo), 100.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (InvalidTargets)
{
    ProcessorPtr obj = new ParamTestNode (1);
    auto node = makeNode (obj);

    // out-of-range parameter index
    ParameterTarget oob (node, 5);
    BOOST_REQUIRE (! oob.isValid());
    oob.apply (MidiMessage::controllerEvent (1, 7, 127), false); // no-op, must not crash

    // invalid node
    Node empty;
    ParameterTarget bad (empty, 0);
    BOOST_REQUIRE (! bad.isValid());
}

BOOST_AUTO_TEST_SUITE_END()
