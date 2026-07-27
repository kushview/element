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
    Signal<void()> tapped;
    int flashes = 0;
    auto conn = tapped.connect ([&flashes] { ++flashes; });
    TempoTarget target (session, shared, tapped);
    BOOST_REQUIRE (target.isValid());

    // Taps are timed by the message timestamp (arrival time in ms), which the
    // router stamps in production. 500 ms apart == 120 BPM.
    auto noteAt = [] (double ms) {
        auto m = MidiMessage::noteOn (1, 60, (uint8) 100);
        m.setTimeStamp (ms);
        return m;
    };

    // First note-on seeds the run; tempo unchanged, but the flash still fires so
    // the very first tap gives visual feedback.
    target.apply (noteAt (0.0), false);
    BOOST_REQUIRE_CLOSE ((double) session.getProperty (tags::tempo), 100.0, 0.0001);
    BOOST_REQUIRE_EQUAL (flashes, 1);

    // Second note-on 500 ms later writes exactly 120 BPM and flashes again.
    target.apply (noteAt (500.0), false);
    BOOST_REQUIRE_CLOSE ((double) session.getProperty (tags::tempo), 120.0, 0.0001);
    BOOST_REQUIRE_EQUAL (flashes, 2);

    // Note-off never taps: tempo stays put and the flash does not fire.
    session.setProperty (tags::tempo, 100.0, nullptr);
    target.apply (MidiMessage::noteOff (1, 60), false);
    BOOST_REQUIRE_CLOSE ((double) session.getProperty (tags::tempo), 100.0, 0.0001);
    BOOST_REQUIRE_EQUAL (flashes, 2);

    conn.disconnect();
}

namespace {

/** A tempo target plus the session tree and flash counter it writes to, so the
    controller cases below stay about the trigger rule and nothing else. */
struct TempoFixture
{
    TempoFixture (const juce::String& mode = "above", int value = 67)
        : target (session, shared, tapped, mode, value)
    {
        session.setProperty (tags::tempo, 100.0, nullptr);
        conn = tapped.connect ([this] { ++flashes; });
    }

    ~TempoFixture() { conn.disconnect(); }

    /** Apply a CC on the mapped controller at the given arrival time. */
    void cc (int value, double ms = 0.0)
    {
        auto m = MidiMessage::controllerEvent (1, 7, value);
        m.setTimeStamp (ms);
        target.apply (m, false);
    }

    double tempo() const { return (double) session.getProperty (tags::tempo); }

    ValueTree session { types::Session };
    TapTempo shared;
    Signal<void()> tapped;
    SignalConnection conn;
    int flashes = 0;
    TempoTarget target;
};

} // namespace

BOOST_AUTO_TEST_CASE (TempoTargetTapsFromCCAboveThreshold)
{
    // Default rule: a tap fires when the value crosses up to or through 67.
    // Twisting back and forth taps once per upward pass, so 500 ms between
    // crossings is 120 BPM.
    TempoFixture f;

    f.cc (0, 0.0); // below the threshold: not a tap
    BOOST_REQUIRE_EQUAL (f.flashes, 0);

    f.cc (80, 100.0); // crossed up: seeds the run
    BOOST_REQUIRE_EQUAL (f.flashes, 1);
    BOOST_REQUIRE_CLOSE (f.tempo(), 100.0, 0.0001);

    f.cc (100, 200.0); // still above: no second tap
    f.cc (20, 400.0); // falling back down: no tap
    BOOST_REQUIRE_EQUAL (f.flashes, 1);

    f.cc (90, 600.0); // crossed up again, 500 ms after the first tap
    BOOST_REQUIRE_EQUAL (f.flashes, 2);
    BOOST_REQUIRE_CLOSE (f.tempo(), 120.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (TempoTargetCCThresholdIsConfigurable)
{
    TempoFixture f ("above", 100);

    f.cc (80, 0.0); // above 67 but below the configured threshold
    BOOST_REQUIRE_EQUAL (f.flashes, 0);

    f.cc (100, 100.0); // exactly at the threshold counts
    BOOST_REQUIRE_EQUAL (f.flashes, 1);
}

BOOST_AUTO_TEST_CASE (TempoTargetCCTouchedZero)
{
    TempoFixture f ("zero");

    f.cc (127, 0.0); // never taps: only arriving at 0 does
    BOOST_REQUIRE_EQUAL (f.flashes, 0);

    f.cc (0, 100.0);
    BOOST_REQUIRE_EQUAL (f.flashes, 1);

    f.cc (0, 200.0); // parked at 0: no repeat
    BOOST_REQUIRE_EQUAL (f.flashes, 1);

    f.cc (64, 500.0);
    f.cc (0, 600.0); // back to 0, 500 ms after the first tap
    BOOST_REQUIRE_EQUAL (f.flashes, 2);
    BOOST_REQUIRE_CLOSE (f.tempo(), 120.0, 0.0001);
}

BOOST_AUTO_TEST_CASE (TempoTargetCCTouchedMax)
{
    TempoFixture f ("max");

    f.cc (0, 0.0);
    BOOST_REQUIRE_EQUAL (f.flashes, 0);

    f.cc (127, 100.0);
    BOOST_REQUIRE_EQUAL (f.flashes, 1);

    f.cc (127, 200.0); // parked at 127: no repeat
    BOOST_REQUIRE_EQUAL (f.flashes, 1);

    f.cc (10, 500.0);
    f.cc (127, 600.0);
    BOOST_REQUIRE_EQUAL (f.flashes, 2);
    BOOST_REQUIRE_CLOSE (f.tempo(), 120.0, 0.0001);
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
