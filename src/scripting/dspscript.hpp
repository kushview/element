// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sol/sol.hpp"

#include <element/parameter.hpp>
#include <element/juce/core.hpp>
#include <element/juce/audio_basics.hpp>

#include "scripting/scriptinstance.hpp"

namespace element {

struct DSPScriptPosition;
class LuaMidiPipe;
class MidiPipe;

class DSPScript : public ScriptInstance
{
public:
    DSPScript (sol::table tbl);
    ~DSPScript();

    /** Calls the script's optional `init` function.
        @return false if the script raised an error (see getLastError)
    */
    bool init();

    /** Compiles and instantiates a DSP script in a scratch Lua state, then
        runs a few cycles of prepare/process/release to catch runtime errors.

        @param script The Lua source of the script.
        @return ok when the script loads, returns a descriptor table, and
                renders without raising or producing non-finite audio.
    */
    static juce::Result validate (const juce::String& script);

    void setPlayHead (juce::AudioPlayHead* ph) noexcept { playhead = ph; }

    /** Returns true if the script loaded ok and has not failed in process */
    bool isValid() const noexcept { return loaded; }

    /** Returns the message of the last error raised by the script, if any. */
    juce::String getLastError() const { return lastError; }

    //==========================================================================
    /** Calls the script's optional `prepare` function.
        @return false if the script raised an error (see getLastError)
    */
    bool prepare (double rate, int block);

    /** Calls the script's optional `release` function.
        @return false if the script raised an error (see getLastError)
    */
    bool release();

    //==========================================================================
    void process (juce::AudioSampleBuffer& a, element::MidiPipe& m);

    //==========================================================================
    void save (juce::MemoryBlock& block);
    void restore (const void* data, size_t size);

    //==========================================================================
    const PortList& getPorts() const { return ports; }
    void getPorts (PortList& out);

    //==========================================================================
    int getNumParameters (bool input = true) const { return input ? numParams : numControls; }
    element::ParameterPtr getParameterObject (int index, bool input = true) const;
    void copyParameterValues (const DSPScript&);

    //==========================================================================
    juce::String getUI() const;

private:
    sol::table DSP;
    sol::function processFunc;
    juce::AudioBuffer<float>** audio = nullptr;
    LuaMidiPipe** midi = nullptr;
    juce::AudioPlayHead* playhead = nullptr;
    DSPScriptPosition** position = nullptr;

    int processRef = LUA_REFNIL;
    int audioRef = LUA_REFNIL;
    int midiRef = LUA_REFNIL;
    int positionRef = LUA_REFNIL;

    lua_State* L = nullptr;
    bool loaded = false;
    juce::String lastError;
    int numParams = 0, // input params
        numControls = 0; // output params
    enum
    {
        maxParams = 128
    };
    float paramData[maxParams],
        controlData[maxParams];
    sol::userdata paramsUserData, controlsUserData;
    PortList ports;

    class Parameter;
    friend class Parameter;
    juce::ReferenceCountedArray<Parameter> inParams, outParams;

    struct Context;
    struct Position;

    void deref();
    void getParameterData (juce::MemoryBlock&, bool);
    void setParameterData (juce::MemoryBlock&, bool);
    void addAudioMidiPorts();
    void addParameter (const sol::table&, bool);
    void addParameterPorts();
    void unlinkParams();
    void setParameter (int, float, bool);

    template <typename... Args>
    bool callOptional (const char* name, Args&&... args);
};

} // namespace element
