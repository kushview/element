// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include "luatest.hpp"
#include "scripting/dspscript.hpp"
#include "scripting/scriptloader.hpp"
#include "testutil.hpp"

using namespace element;

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

BOOST_AUTO_TEST_SUITE_END()
