// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "sol/sol.hpp"
#include <element/parameter.hpp>
#include <element/juce/core.hpp>

#include "scripting/scriptinstance.hpp"

namespace element {

class LuaMidiPipe;

class DSPScript : public ScriptInstance
{
public:
    DSPScript (sol::table tbl);
    ~DSPScript();

    void init()
    {
        if (sol::function f = DSP["init"])
            f();
    }

    static juce::Result validate (const juce::String& script);

    /** Returns true if the script loaded ok */
    bool isValid() const { return loaded; }

    //==========================================================================
    void prepare (double rate, int block)
    {
        if (sol::function f = DSP["prepare"])
            f (rate, block);
    }

    void release()
    {
        if (sol::function f = DSP["release"])
            f();
    }

    //==========================================================================
    void process (juce::AudioSampleBuffer& a, element::MidiPipe& m);

    //==========================================================================
    void save (juce::MemoryBlock& block);
    void restore (const void* data, size_t size);

    //==========================================================================
    const PortList& getPorts() const { return ports; }
    void getPorts (PortList& out);

    //==========================================================================
    int getNumParameters() const { return numParams; }
    element::ParameterPtr getParameterObject (int index, bool input = true) const;
    void copyParameterValues (const DSPScript&);

    //==========================================================================
    juce::String getUI() const;

private:
    sol::table DSP;
    sol::function processFunc;
    AudioBuffer<float>** audio = nullptr;
    LuaMidiPipe** midi = nullptr;
    int processRef = LUA_REFNIL;
    int audioRef = LUA_REFNIL;
    int midiRef = LUA_REFNIL;
    lua_State* L = nullptr;
    bool loaded = false;
    int numParams = 0;
    enum
    {
        maxParams = 128
    };
    float paramData[maxParams];
    sol::userdata params;
    PortList ports;

    class Parameter;
    friend class Parameter;
    juce::ReferenceCountedArray<Parameter> inParams, outParams;

    void deref();
    void getParameterData (juce::MemoryBlock&);
    void setParameterData (juce::MemoryBlock&);
    void addAudioMidiPorts();
    void addParameterPorts();
    void unlinkParams();
    void setParameter (int, float);
};

} // namespace element
