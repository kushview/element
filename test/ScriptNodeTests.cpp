// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include "luascripts.hpp"
#include "nodes/scriptnode.hpp"

using namespace element;
using namespace juce;

namespace {

/** Compiles fine and passes layout, but raises in process (the report that
    used to terminate the app: an undefined global on the audio thread). */
const char* brokenScript = R"(
-- @script broken
-- @type DSP
local function layout() return { audio = { 2, 2 }, midi = { 0, 0 } } end
local function process (a, m, p, c, t) a2:fade (1.0, 1.0) end
return { type = 'DSP', layout = layout, process = process }
)";

String ampScript()
{
    return String::fromUTF8 (scripts::amp_lua, scripts::amp_luaSize);
}

MemoryBlock makeState (const String& dspCode)
{
    ValueTree state ("ScriptNode");
    state.setProperty ("dspCode", dspCode, nullptr)
        .setProperty ("editorCode", String(), nullptr);
    MemoryBlock out;
    MemoryOutputStream mo (out, false);
    {
        GZIPCompressorOutputStream gz (mo);
        state.writeToStream (gz);
    }
    return out;
}

} // namespace

BOOST_AUTO_TEST_SUITE (ScriptNodeTests)

BOOST_AUTO_TEST_CASE (DefaultScriptLoadsClean)
{
    ScriptNode::Ptr node = new ScriptNode();
    BOOST_REQUIRE (node->getScriptError().isEmpty());
}

BOOST_AUTO_TEST_CASE (LoadScriptRecordsAndClearsError)
{
    ScriptNode::Ptr node = new ScriptNode();

    auto result = node->loadScript (brokenScript);
    BOOST_REQUIRE (result.failed());
    BOOST_REQUIRE_EQUAL (node->getScriptError().toStdString(), result.getErrorMessage().toStdString());
    BOOST_REQUIRE (node->getScriptError().contains ("a2"));

    result = node->loadScript (ampScript());
    BOOST_REQUIRE_MESSAGE (result.wasOk(), result.getErrorMessage().toStdString());
    BOOST_REQUIRE (node->getScriptError().isEmpty());
}

BOOST_AUTO_TEST_CASE (SetStateWithBrokenScriptRecordsError)
{
    ScriptNode::Ptr node = new ScriptNode();
    const auto state = makeState (brokenScript);
    node->setState (state.getData(), (int) state.getSize());

    BOOST_REQUIRE (node->getScriptError().contains ("a2"));
    // The offending code is kept so the editor can show and fix it.
    BOOST_REQUIRE (node->getCodeDocument().getAllContent().contains ("a2:fade"));
}

BOOST_AUTO_TEST_CASE (GraphActivationCallbackRunsAtRenderBoundary)
{
    ScriptNode::Ptr node = new ScriptNode();
    const auto result = node->loadScript (R"(
        local midi = require ('el.midi')
        local function layout() return { audio = { 2, 2 }, midi = { 1, 1 } } end
        local function on_graph_activated (a, m, p, c, t)
            m:get (1):insertPacked (midi.controller (1, 74, 100), 1)
        end
        local function process (a, m, p, c, t) end
        return {
            type = 'DSP',
            layout = layout,
            on_graph_activated = on_graph_activated,
            process = process
        }
    )");
    BOOST_REQUIRE_MESSAGE (result.wasOk(), result.getErrorMessage().toStdString());

    AudioSampleBuffer audio (2, 64), cv (1, 64);
    MidiBuffer midi;
    RenderContext rc (audio, cv, midi, 64);

    node->graphActivated();
    node->render (rc);
    BOOST_REQUIRE_EQUAL (midi.getNumEvents(), 1);
    const auto activationMessage = (*midi.begin()).getMessage();
    BOOST_REQUIRE (activationMessage.isController());
    BOOST_REQUIRE_EQUAL (activationMessage.getControllerNumber(), 74);

    midi.clear();
    node->render (rc);
    BOOST_REQUIRE (midi.isEmpty());
}

BOOST_AUTO_TEST_SUITE_END()
