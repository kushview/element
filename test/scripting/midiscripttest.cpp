// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include "luatest.hpp"

BOOST_AUTO_TEST_SUITE (MidiScriptTests)

namespace bdata = boost::unit_test::data;
using JuceCounterpart = std::function<juce::MidiMessage()>;

struct LuaMidiTestCase {
    std::string luaMethodCall;
    JuceCounterpart juceCounterpart;
    bool shouldCheckTheSecondByte = true;
    bool shouldCheckThirdByte = true;

    friend std::ostream& operator<< (std::ostream& os, const LuaMidiTestCase& testCase)
    {
        os << "{ luaMethodCall: \"" << testCase.luaMethodCall << "\", ... } ";
        return os;
    }
};

BOOST_DATA_TEST_CASE (
    luaSimpleBindigs,
    bdata::make (std::initializer_list<LuaMidiTestCase> {
        { .luaMethodCall = "return midi.controller(1, 7, 120)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::controllerEvent (1, 7, 120); } },

        { .luaMethodCall = "return midi.noteon(1, 60, 100)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOn (1, 60, (uint8_t) 100); } },

        { .luaMethodCall = "return midi.noteoff(1, 60)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOff (1, 60); } },

        { .luaMethodCall = "return midi.program(1, 5)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::programChange (1, 5); },
          .shouldCheckThirdByte = false },

        { .luaMethodCall = "return midi.pitch(1, 8192)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::pitchWheel (1, 8192); } },

        { .luaMethodCall = "return midi.aftertouch(1, 60, 64)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::aftertouchChange (1, 60, 64); } },

        { .luaMethodCall = "return midi.channelpressure(1, 45)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::channelPressureChange (1, 45); },
          .shouldCheckThirdByte = false },

        { .luaMethodCall = "return midi.allnotesoff(1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allNotesOff (1); } },

        { .luaMethodCall = "return midi.allsoundsoff(1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allSoundOff (1); } },

        { .luaMethodCall = "return midi.allcontrollersoff(1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allControllersOff (1); } },

        { .luaMethodCall = "return midi.clock()",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::midiClock(); },
          .shouldCheckTheSecondByte = false,
          .shouldCheckThirdByte = false },

        { .luaMethodCall = "return midi.start()",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::midiStart(); },
          .shouldCheckTheSecondByte = false,
          .shouldCheckThirdByte = false },

        { .luaMethodCall = "return midi.stop()",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::midiStop(); },
          .shouldCheckTheSecondByte = false,
          .shouldCheckThirdByte = false },

        { .luaMethodCall = "return midi.continue()",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::midiContinue(); },
          .shouldCheckTheSecondByte = false,
          .shouldCheckThirdByte = false },
    }),
    testData)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());
    lua.script ("midi = require('el.midi')");

    int64_t packed = lua.script (testData.luaMethodCall);

    auto expected = testData.juceCounterpart();
    BOOST_CHECK_EQUAL (uint8_t (packed), expected.getRawData()[0]);
    if (testData.shouldCheckTheSecondByte) {
        BOOST_CHECK_EQUAL (uint8_t (packed >> 8), expected.getRawData()[1]);
    } else {
        BOOST_CHECK_EQUAL (uint8_t (packed >> 8), 0);
    }
    if (testData.shouldCheckThirdByte) {
        BOOST_CHECK_EQUAL (uint8_t (packed >> 16), expected.getRawData()[2]);
    } else {
        BOOST_CHECK_EQUAL (uint8_t (packed >> 16), 0);
    }
}

BOOST_AUTO_TEST_CASE (test_tohertz)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());
    lua.script ("midi = require('el.midi')");

    for (int note = 0; note <= 127; ++note) {
        std::string script = "return midi.tohertz(" + std::to_string (note) + ")";
        double luaFreq = lua.script (script);

        double juceFreq = juce::MidiMessage::getMidiNoteInHertz (note);

        BOOST_CHECK_CLOSE (luaFreq, juceFreq, 0.001);
    }
}

BOOST_AUTO_TEST_CASE (test_clamp)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());

    lua.script ("midi = require('el.midi')");

    int64_t resultNormal = lua.script ("return midi.clamp(64)");
    BOOST_CHECK_EQUAL (resultNormal, 64.0);

    int64_t resultMin = lua.script ("return midi.clamp(0)");
    BOOST_CHECK_EQUAL (resultMin, 0);

    int64_t resultMax = lua.script ("return midi.clamp(127)");
    BOOST_CHECK_EQUAL (resultMax, 127);

    int64_t resultUnder = lua.script ("return midi.clamp(-50)");
    BOOST_CHECK_EQUAL (resultUnder, 0);

    int64_t resultOver = lua.script ("return midi.clamp(200)");
    BOOST_CHECK_EQUAL (resultOver, 127);
}
BOOST_AUTO_TEST_SUITE_END()