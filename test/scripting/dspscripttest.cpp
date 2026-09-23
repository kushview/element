// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include "luatest.hpp"
#include "luascripts.hpp"
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "testutil.hpp"

using namespace element;
using namespace juce;

BOOST_AUTO_TEST_SUITE (DSPScriptTest)

BOOST_AUTO_TEST_CASE (Basics)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());

    auto script = std::make_unique<ScriptLoader> (lua);
    script->load (fix.getSnippetFile ("test_dsp_script_01.lua"));
    if (script->hasError()) {
        BOOST_REQUIRE_MESSAGE (false,
                               (String ("Could not load script: ") + script->getErrorMessage())
                                   .toStdString());
        return;
    }

    BOOST_REQUIRE_MESSAGE (! script->hasError(),
                           script->getErrorMessage().toStdString());
    if (script->hasError())
        return;

    auto result = script->call();
    BOOST_REQUIRE_MESSAGE (result.get_type() == sol::type::table,
                           sol::type_name (result.lua_state(), result.get_type()));

    sol::table Amp = result;
    DSPScript dsp (Amp);

    BOOST_REQUIRE_MESSAGE (dsp.isValid(), "Could not instantiate DSP Script");
    if (! dsp.isValid())
        return;

#define expect    BOOST_REQUIRE
#define expectMsg BOOST_REQUIRE_MESSAGE

    const auto& ports = dsp.getPorts();
    expect (ports.size (PortType::Audio, true) == 2);
    expect (ports.size (PortType::Audio, false) == 2);
    expect (ports.size (PortType::Midi, true) == 0);
    expect (ports.size (PortType::Midi, false) == 0);
    expect (ports.size (PortType::Control, true) == 1);
    expect (ports.size (PortType::Control, false) == 0);

    dsp.init();
    expectMsg (Amp.get_or ("initialized", false), "didn't call init");

    dsp.prepare (44100, 4096);
    expectMsg (Amp.get_or ("rate", 0.0) == 44100.0, String (Amp.get_or ("rate", 0)));
    expectMsg (Amp.get_or ("block", 0.0) == 4096.0, String (Amp.get_or ("block", 0)));

    AudioSampleBuffer audio (2, 4096);
    for (int c = 0; c < 2; ++c)
        for (int f = 0; f < 4096; ++f)
            audio.setSample (c, f, 1.0);
    MidiPipe midi;

    dsp.getParameterObject (0)->setValueNotifyingHost (0.0);
    dsp.process (audio, midi);

    expectMsg (audio.getSample (0, 4095) < 1.0, String (audio.getSample (0, 4095)));

    MemoryBlock block;
    dsp.save (block);
    dsp.restore (block.getData(), block.getSize());

    expect (Amp.get_or ("released", true) == false);
    dsp.release();
    expect (Amp.get_or ("released", false) == true);
}

static juce::String dspScriptWithProcess (const char* body)
{
    return juce::String (R"(
--- Validate fixture.
-- @script validate_fixture
-- @type DSP
local function layout() return { audio = { 2, 2 }, midi = { 1, 1 } } end
local function process (a, m, p, c, t)
)") + body
           + R"(
end
return { type = 'DSP', layout = layout, process = process }
)";
}

BOOST_AUTO_TEST_CASE (ValidateShippedAmp)
{
    const auto amp = String::fromUTF8 (scripts::amp_lua, scripts::amp_luaSize);
    auto result = DSPScript::validate (amp);
    BOOST_REQUIRE_MESSAGE (result.wasOk(), result.getErrorMessage().toStdString());
}

BOOST_AUTO_TEST_CASE (ValidateRejectsEmpty)
{
    BOOST_REQUIRE (DSPScript::validate ("").failed());
}

BOOST_AUTO_TEST_CASE (ValidateRejectsSyntaxError)
{
    BOOST_REQUIRE (DSPScript::validate ("return {").failed());
}

BOOST_AUTO_TEST_CASE (ValidateRejectsNonTable)
{
    BOOST_REQUIRE (DSPScript::validate ("return 42").failed());
}

BOOST_AUTO_TEST_CASE (ValidateRendersMidiAndAudio)
{
    auto result = DSPScript::validate (dspScriptWithProcess (R"(
        local buf = m:get (1)
        assert (buf:size() > 0, 'expected midi events')
        a:fade (1.0, 0.5)
    )"));
    BOOST_REQUIRE_MESSAGE (result.wasOk(), result.getErrorMessage().toStdString());
}

BOOST_AUTO_TEST_CASE (ValidateRejectsProcessError)
{
    auto result = DSPScript::validate (dspScriptWithProcess ("error ('process exploded')"));
    BOOST_REQUIRE (result.failed());
    BOOST_REQUIRE_MESSAGE (result.getErrorMessage().contains ("process exploded"),
                           result.getErrorMessage().toStdString());
}

BOOST_AUTO_TEST_CASE (ValidateRejectsNonFiniteOutput)
{
    auto result = DSPScript::validate (dspScriptWithProcess ("a:fade (math.huge, math.huge)"));
    BOOST_REQUIRE (result.failed());
    BOOST_REQUIRE_MESSAGE (result.getErrorMessage().contains ("non-finite"),
                           result.getErrorMessage().toStdString());
}

BOOST_AUTO_TEST_CASE (ProcessErrorDisablesScriptInsteadOfAborting)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());
    ScriptLoader loader (lua, dspScriptWithProcess ("error ('boom')"));
    BOOST_REQUIRE_MESSAGE (! loader.hasError(), loader.getErrorMessage().toStdString());
    sol::table descriptor = loader.call();
    DSPScript dsp (descriptor);
    BOOST_REQUIRE (dsp.isValid());

    AudioSampleBuffer audio (2, 64);
    MidiPipe midi;
    dsp.process (audio, midi);
    BOOST_REQUIRE (! dsp.isValid());
    BOOST_REQUIRE (dsp.getLastError().contains ("boom"));

    // A disabled script is a no-op afterwards, not a crash.
    dsp.process (audio, midi);
}

BOOST_AUTO_TEST_SUITE_END()
