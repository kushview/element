// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <functional>
#include "luatest.hpp"
#include "el/bytes.h"

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
        // --- controller ---
        { .luaMethodCall = "return midi.controller(1, 7, 120)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::controllerEvent (1, 7, 120); } },
        { .luaMethodCall = "return midi.controller(10, 1, 0)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::controllerEvent (10, 1, 0); } },
        { .luaMethodCall = "return midi.controller(16, 64, 127)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::controllerEvent (16, 64, 127); } },

        // --- noteon ---
        { .luaMethodCall = "return midi.noteon(1, 60, 100)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOn (1, 60, (uint8_t) 100); } },
        { .luaMethodCall = "return midi.noteon(10, 36, 127)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOn (10, 36, (uint8_t) 127); } },
        { .luaMethodCall = "return midi.noteon(16, 0, 1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOn (16, 0, (uint8_t) 1); } },

        // --- noteoff ---
        { .luaMethodCall = "return midi.noteoff(1, 60)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOff (1, 60); } },
        { .luaMethodCall = "return midi.noteoff(10, 36)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOff (10, 36); } },
        { .luaMethodCall = "return midi.noteoff(16, 127)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::noteOff (16, 127); } },

        // --- program ---
        { .luaMethodCall = "return midi.program(1, 5)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::programChange (1, 5); },
          .shouldCheckThirdByte = false },
        { .luaMethodCall = "return midi.program(10, 0)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::programChange (10, 0); },
          .shouldCheckThirdByte = false },
        { .luaMethodCall = "return midi.program(16, 127)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::programChange (16, 127); },
          .shouldCheckThirdByte = false },

        // --- pitch ---
        { .luaMethodCall = "return midi.pitch(1, 8192)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::pitchWheel (1, 8192); } },
        { .luaMethodCall = "return midi.pitch(10, 0)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::pitchWheel (10, 0); } },
        { .luaMethodCall = "return midi.pitch(16, 16383)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::pitchWheel (16, 16383); } },

        // --- aftertouch ---
        { .luaMethodCall = "return midi.aftertouch(1, 60, 64)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::aftertouchChange (1, 60, 64); } },
        { .luaMethodCall = "return midi.aftertouch(10, 48, 0)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::aftertouchChange (10, 48, 0); } },
        { .luaMethodCall = "return midi.aftertouch(16, 127, 127)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::aftertouchChange (16, 127, 127); } },

        // --- channelpressure ---
        { .luaMethodCall = "return midi.channelpressure(1, 45)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::channelPressureChange (1, 45); },
          .shouldCheckThirdByte = false },
        { .luaMethodCall = "return midi.channelpressure(10, 0)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::channelPressureChange (10, 0); },
          .shouldCheckThirdByte = false },
        { .luaMethodCall = "return midi.channelpressure(16, 127)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::channelPressureChange (16, 127); },
          .shouldCheckThirdByte = false },

        // --- allnotesoff ---
        { .luaMethodCall = "return midi.allnotesoff(1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allNotesOff (1); } },
        { .luaMethodCall = "return midi.allnotesoff(10)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allNotesOff (10); } },
        { .luaMethodCall = "return midi.allnotesoff(16)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allNotesOff (16); } },

        // --- allsoundsoff ---
        { .luaMethodCall = "return midi.allsoundsoff(1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allSoundOff (1); } },
        { .luaMethodCall = "return midi.allsoundsoff(10)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allSoundOff (10); } },
        { .luaMethodCall = "return midi.allsoundsoff(16)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allSoundOff (16); } },

        // --- allcontrollersoff ---
        { .luaMethodCall = "return midi.allcontrollersoff(1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allControllersOff (1); } },
        { .luaMethodCall = "return midi.allcontrollersoff(10)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allControllersOff (10); } },
        { .luaMethodCall = "return midi.allcontrollersoff(16)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::allControllersOff (16); } },

        // --- system messages (no channel) ---
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

        { .luaMethodCall = "return midi.songpositionpointer(12)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::songPositionPointer (12); },
          .shouldCheckTheSecondByte = true,
          .shouldCheckThirdByte = true },

        { .luaMethodCall = "return midi.quarterframe(2, 14)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::quarterFrame (2, 14); },
          .shouldCheckTheSecondByte = true,
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

struct LuaMtcFullFrameTestCase {
    std::string luaMethodCall;
    std::string luaTransportMethodCall;
    JuceCounterpart juceCounterpart;

    friend std::ostream& operator<< (std::ostream& os, const LuaMtcFullFrameTestCase& testCase)
    {
        os << "{ luaMethodCall: \"" << testCase.luaMethodCall << "\", ... } ";
        return os;
    }
};

BOOST_DATA_TEST_CASE (
    mtcFullFrame,
    bdata::make (std::initializer_list<LuaMtcFullFrameTestCase> {
        // --- rate types (all time fields zero, currentSample = 0) ---
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 0, 0, 0, 0)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 0, 44100, 24)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 0, 0, 0, juce::MidiMessage::fps24); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 0, 0, 0, 1)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 0, 44100, 25)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 0, 0, 0, juce::MidiMessage::fps25); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 0, 0, 0, 2)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 0, 44100, 29)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 0, 0, 0, juce::MidiMessage::fps30drop); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 0, 0, 0, 3)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 0, 44100, 30)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 0, 0, 0, juce::MidiMessage::fps30); } },

        // --- boundary values (23:59:59:29 at 30fps) ---
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 23, 59, 59, 29, 3)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 3810238530, 44100, 30)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (23, 59, 59, 29, juce::MidiMessage::fps30); } },

        // --- non-zero time fields (24fps) ---
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 1, 0, 0, 0, 0)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 158760000, 44100, 24)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (1, 0, 0, 0, juce::MidiMessage::fps24); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 30, 0, 0, 0)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 79380000, 44100, 24)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 30, 0, 0, juce::MidiMessage::fps24); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 0, 45, 0, 0)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 1984500, 44100, 24)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 0, 45, 0, juce::MidiMessage::fps24); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 0, 0, 0, 15, 0)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 27563, 44100, 24)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (0, 0, 0, 15, juce::MidiMessage::fps24); } },

        // --- 17:30:45:12 with different rate types (each with fps-specific sample position) ---
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 17, 30, 45, 12, 0)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 2780306550, 44100, 24)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (17, 30, 45, 12, juce::MidiMessage::fps24); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 17, 30, 45, 12, 1)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 2780305668, 44100, 25)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (17, 30, 45, 12, juce::MidiMessage::fps25); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 17, 30, 45, 12, 2)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 2780302749, 44100, 29)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (17, 30, 45, 12, juce::MidiMessage::fps30drop); } },
        { .luaMethodCall = "return midi.mtcfullframe(buffer, 17, 30, 45, 12, 3)",
          .luaTransportMethodCall = "return midi.mtcfullframetransport(buffer, 2780302140, 44100, 30)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage::fullFrame (17, 30, 45, 12, juce::MidiMessage::fps30); } },

    }),
    testData)
{
    LuaFixture fix;
    lua_State* L = fix.luaState();
    sol::state_view lua (L);
    lua.script ("midi = require('el.midi')");
    lua.script ("bytes = require ('el.bytes')");
    lua.script ("buffer = bytes.new(10)");

    lua_getglobal(L, "buffer");
    auto* bytes = (EL_Bytes*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    BOOST_REQUIRE_MESSAGE(bytes != nullptr, "Failed to read EL_Bytes userdata from Lua!");

    auto expected = testData.juceCounterpart();
    const int expectedSize = expected.getRawDataSize();

    // Check mtcfullframe against JUCE
    luaL_dostring (L, testData.luaMethodCall.c_str());
    lua_Integer size = lua_tointeger (L, -1);

    BOOST_CHECK_EQUAL (size, expectedSize);
    for (int i = 0; i < size && i < expectedSize; ++i) {
        BOOST_CHECK_EQUAL (bytes->data[i], expected.getRawData()[i]);
    }

    std::fill (bytes->data, bytes->data + bytes->size, 0);

    // Check mtcfullframetransport against JUCE
    luaL_dostring (L, testData.luaTransportMethodCall.c_str());
    lua_Integer transportSize = lua_tointeger (L, -1);

    BOOST_CHECK_EQUAL (transportSize, expectedSize);
    for (int i = 0; i < transportSize && i < expectedSize; ++i) {
        BOOST_CHECK_EQUAL (bytes->data[i], expected.getRawData()[i]);
    }
}

struct LuaQuarterFrameTestCase {
    std::string luaMethodCall;
    JuceCounterpart juceCounterpart;
    lua_Integer expectedRelativeSampleFrame;
    lua_Integer expectedNumberOfMessagesToSent;
    friend std::ostream& operator<< (std::ostream& os, const LuaQuarterFrameTestCase& testCase)
    {
        os << "{ luaMethodCall: \"" << testCase.luaMethodCall << "\", ... } ";
        return os;
    }
};

const auto noMessage = []() -> juce::MidiMessage { return juce::MidiMessage { 0, 0 }; };
BOOST_DATA_TEST_CASE (
    quarterframetransport,
    bdata::make (std::initializer_list<LuaQuarterFrameTestCase> {
        // --- frames at t=0 (seq 0-1, multiple messages per block, no-message case) ---
        { .luaMethodCall = "return midi.quarterframetransport(0, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 0;
              constexpr int framesLowNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, framesLowNibble);
          },
          .expectedRelativeSampleFrame = 0,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(0, 512, 44100, 24, 1)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 1;
              constexpr int framesHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, framesHighNibble);
          },
          .expectedRelativeSampleFrame = 459,
          .expectedNumberOfMessagesToSent = 2 },

        { .luaMethodCall = "return midi.quarterframetransport(0, 512, 44100, 24, 2)",
          .juceCounterpart = noMessage,
          .expectedRelativeSampleFrame = 0,
          .expectedNumberOfMessagesToSent = 2 },

        // --- seconds at t=0 (seq 2-3, includes no-message case) ---
        { .luaMethodCall = "return midi.quarterframetransport(512, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 2;
              constexpr int secondsLowNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, secondsLowNibble);
          },
          .expectedRelativeSampleFrame = 406,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(512, 512, 44100, 24, 1)",
          .juceCounterpart = noMessage,
          .expectedRelativeSampleFrame = 0,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(1024, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 3;
              constexpr int secondsHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, secondsHighNibble);
          },
          .expectedRelativeSampleFrame = 354,
          .expectedNumberOfMessagesToSent = 1 },

        // --- minutes at t=0 (seq 4-5) ---
        { .luaMethodCall = "return midi.quarterframetransport(1536, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 4;
              constexpr int minutesLowNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, minutesLowNibble);
          },
          .expectedRelativeSampleFrame = 301,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(2048, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 5;
              constexpr int minutesHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, minutesHighNibble);
          },
          .expectedRelativeSampleFrame = 248,
          .expectedNumberOfMessagesToSent = 1 },

        // --- hours at t=0 (seq 6-7) ---
        { .luaMethodCall = "return midi.quarterframetransport(2560, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 6;
              constexpr int hoursLowNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, hoursLowNibble);
          },
          .expectedRelativeSampleFrame = 196,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(3072, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 7;
              constexpr int hoursHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, hoursHighNibble);
          },
          .expectedRelativeSampleFrame = 143,
          .expectedNumberOfMessagesToSent = 1 },

        // --- wrap-around: second MTC frame cycle (seq 0-1 again, framesLowNibble = 2) ---
        { .luaMethodCall = "return midi.quarterframetransport(3584, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 0;
              constexpr int framesLowNibble = 2;
              return juce::MidiMessage::quarterFrame (messageNumber, framesLowNibble);
          },
          .expectedRelativeSampleFrame = 91,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(3584, 512, 44100, 24, 1)",
          .juceCounterpart = []() -> juce::MidiMessage { return juce::MidiMessage (0, 0); },
          .expectedRelativeSampleFrame = 0,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(4096, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 1;
              constexpr int framesHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, framesHighNibble);
          },
          .expectedRelativeSampleFrame = 38,
          .expectedNumberOfMessagesToSent = 1 },

        // --- ~1s elapsed (seq 1-3, secondsLowNibble = 1) ---
        { .luaMethodCall = "return midi.quarterframetransport(44544, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 1;
              constexpr int framesHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, framesHighNibble);
          },
          .expectedRelativeSampleFrame = 15,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(44544, 512, 44100, 24, 1)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 2;
              constexpr int secondsLowNibble = 1;
              return juce::MidiMessage::quarterFrame (messageNumber, secondsLowNibble);
          },
          .expectedRelativeSampleFrame = 474,
          .expectedNumberOfMessagesToSent = 2 },

        { .luaMethodCall = "return midi.quarterframetransport(45056, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 3;
              constexpr int secondsHighNibble = 0;
              return juce::MidiMessage::quarterFrame (messageNumber, secondsHighNibble);
          },
          .expectedRelativeSampleFrame = 422,
          .expectedNumberOfMessagesToSent = 1 },

        // --- ~1min elapsed (seq 4, minutesHighNibble = 1) ---
        { .luaMethodCall = "return midi.quarterframetransport(2647512, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 4;
              constexpr int minutesHighNibble = 1;
              return juce::MidiMessage::quarterFrame (messageNumber, minutesHighNibble);
          },
          .expectedRelativeSampleFrame = 325,
          .expectedNumberOfMessagesToSent = 1 },

        // --- hours (17h elapsed: 0x11, low nibble = 1, high nibble = 1) ---
        { .luaMethodCall = "return midi.quarterframetransport(2698922500, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 6;
              constexpr int hoursLowNibble = 1;
              return juce::MidiMessage::quarterFrame (messageNumber, hoursLowNibble);
          },
          .expectedRelativeSampleFrame = 256,
          .expectedNumberOfMessagesToSent = 1 },

        { .luaMethodCall = "return midi.quarterframetransport(2698923000, 512, 44100, 24, 0)",
          .juceCounterpart = []() -> juce::MidiMessage {
              constexpr int messageNumber = 7;
              constexpr int hoursHighNibble = 1;
              return juce::MidiMessage::quarterFrame (messageNumber, hoursHighNibble);
          },
          .expectedRelativeSampleFrame = 215,
          .expectedNumberOfMessagesToSent = 1 },

    }),
    testData)
{
    LuaFixture fix;
    sol::state_view lua (fix.luaState());

    lua.script ("midi = require('el.midi')");
    auto [numberOfMessagesToSend, packed, frame] = lua.script (testData.luaMethodCall).template get<std::tuple<lua_Integer, lua_Integer, lua_Integer>>();

    auto juceQuarterFrame = testData.juceCounterpart();

    BOOST_CHECK_EQUAL (uint8_t (packed), juceQuarterFrame.getRawData()[0]);
    BOOST_CHECK_EQUAL (uint8_t (packed >> 8), juceQuarterFrame.getRawData()[1]);
    BOOST_CHECK_EQUAL (uint8_t (packed >> 16), 0);
    BOOST_CHECK_EQUAL (frame, testData.expectedRelativeSampleFrame);
    BOOST_CHECK_EQUAL (numberOfMessagesToSend, testData.expectedNumberOfMessagesToSent);
}
BOOST_AUTO_TEST_SUITE_END()