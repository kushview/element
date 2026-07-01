// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for the built-in Script node preset scripts shipped in scripts/.
// Loads the real .lua files from the source tree, instantiates them as
// DSPScripts, and exercises their ports and processing.

#include <boost/test/unit_test.hpp>

#include <element/midipipe.hpp>
#include <element/parameter.hpp>

#include "luatest.hpp"
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "testutil.hpp"

using namespace element;
using namespace juce;

namespace {

/** Returns a File pointing at one of the in-tree preset scripts. */
static File presetScript (const String& filename)
{
    return File (EL_TEST_SOURCE_ROOT)
        .getChildFile ("scripts")
        .getChildFile (filename);
}

/** Casts a script parameter object to its concrete RangedParameter type. */
static RangedParameter* ranged (const ParameterPtr& p)
{
    return dynamic_cast<RangedParameter*> (p.get());
}

} // namespace

BOOST_AUTO_TEST_SUITE (PresetScriptsTest)

//=============================================================================
// Dial: a single control output 'value' (0.0 - 1.0), no inputs.
BOOST_AUTO_TEST_CASE (DialPorts)
{
    LuaFixture fix;

    ScriptLoader loader (fix.luaState());
    loader.load (presetScript ("dial.lua"));
    BOOST_REQUIRE_MESSAGE (! loader.hasError(), loader.getErrorMessage().toStdString());

    auto result = loader.call();
    BOOST_REQUIRE (result.get_type() == sol::type::table);
    sol::table tbl = result;

    DSPScript dsp (tbl);
    BOOST_REQUIRE_MESSAGE (dsp.isValid(), "Could not instantiate dial.lua");

    const auto& ports = dsp.getPorts();
    BOOST_CHECK_EQUAL (ports.size (PortType::Audio, true), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Audio, false), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Midi, true), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Midi, false), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Control, true), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Control, false), 1);

    auto* value = ranged (dsp.getParameterObject (0, false));
    BOOST_REQUIRE (value != nullptr);
    BOOST_CHECK_EQUAL (value->getPort().symbol.toStdString(), std::string ("value"));

    const auto& range = value->getNormalisableRange();
    BOOST_CHECK_EQUAL (range.start, 0.0f);
    BOOST_CHECK_EQUAL (range.end, 1.0f);
    BOOST_CHECK_CLOSE (value->get(), 0.5f, 0.0001);
}

//=============================================================================
// MIDI CC: inputs value/cc/channel, a 0-127 control output, and a MIDI out.
BOOST_AUTO_TEST_CASE (MidiCCPorts)
{
    LuaFixture fix;

    ScriptLoader loader (fix.luaState());
    loader.load (presetScript ("midicc.lua"));
    BOOST_REQUIRE_MESSAGE (! loader.hasError(), loader.getErrorMessage().toStdString());

    auto result = loader.call();
    BOOST_REQUIRE (result.get_type() == sol::type::table);
    sol::table tbl = result;

    DSPScript dsp (tbl);
    BOOST_REQUIRE_MESSAGE (dsp.isValid(), "Could not instantiate midicc.lua");

    const auto& ports = dsp.getPorts();
    BOOST_CHECK_EQUAL (ports.size (PortType::Audio, true), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Audio, false), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Midi, true), 0);
    BOOST_CHECK_EQUAL (ports.size (PortType::Midi, false), 1);
    BOOST_CHECK_EQUAL (ports.size (PortType::Control, true), 3);
    BOOST_CHECK_EQUAL (ports.size (PortType::Control, false), 1);
}

//=============================================================================
// MIDI CC processing: normalized value scales to 0-127 on both the MIDI CC
// message and the control output, and messages are only sent when it changes.
BOOST_AUTO_TEST_CASE (MidiCCProcess)
{
    LuaFixture fix;

    ScriptLoader loader (fix.luaState());
    loader.load (presetScript ("midicc.lua"));
    BOOST_REQUIRE_MESSAGE (! loader.hasError(), loader.getErrorMessage().toStdString());

    auto result = loader.call();
    sol::table tbl = result;
    DSPScript dsp (tbl);
    BOOST_REQUIRE_MESSAGE (dsp.isValid(), "Could not instantiate midicc.lua");

    dsp.prepare (44100.0, 64);

    auto setInput = [&] (int index, float human) {
        auto* p = ranged (dsp.getParameterObject (index, true));
        BOOST_REQUIRE (p != nullptr);
        p->set (human);
    };

    // value = 1.0 (-> 127), cc number = 7, channel = 2.
    setInput (0, 1.0f);
    setInput (1, 7.0f);
    setInput (2, 2.0f);

    AudioSampleBuffer audio (1, 64);
    audio.clear();

    auto runProcess = [&]() -> MidiBuffer {
        MidiBuffer buffer;
        MidiPipe pipe (buffer);
        dsp.process (audio, pipe);
        return buffer;
    };

    // First block: a CC message is emitted and the output control reads 127.
    {
        auto buffer = runProcess();

        int count = 0;
        MidiMessage msg;
        for (const auto meta : buffer)
        {
            msg = meta.getMessage();
            ++count;
        }

        BOOST_CHECK_EQUAL (count, 1);
        BOOST_CHECK (msg.isController());
        BOOST_CHECK_EQUAL (msg.getChannel(), 2);
        BOOST_CHECK_EQUAL (msg.getControllerNumber(), 7);
        BOOST_CHECK_EQUAL (msg.getControllerValue(), 127);

        auto* out = ranged (dsp.getParameterObject (0, false));
        BOOST_REQUIRE (out != nullptr);
        BOOST_CHECK_CLOSE (out->get(), 127.0f, 0.0001);
    }

    // Second block with no change: nothing is emitted.
    {
        auto buffer = runProcess();
        BOOST_CHECK (buffer.isEmpty());
    }

    // Changing the value emits a new message with the new scaled value.
    {
        setInput (0, 0.0f);
        auto buffer = runProcess();

        int count = 0;
        MidiMessage msg;
        for (const auto meta : buffer)
        {
            msg = meta.getMessage();
            ++count;
        }

        BOOST_CHECK_EQUAL (count, 1);
        BOOST_CHECK_EQUAL (msg.getControllerValue(), 0);

        auto* out = ranged (dsp.getParameterObject (0, false));
        BOOST_REQUIRE (out != nullptr);
        BOOST_CHECK_CLOSE (out->get(), 0.0f, 0.0001);
    }
}

BOOST_AUTO_TEST_SUITE_END()
