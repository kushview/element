/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "sol/sol.hpp"
#include "engine/Parameter.h"
#include "scripting/ScriptInstance.h"
#include "JuceHeader.h"

namespace Element {

class LuaMidiPipe;

class DSPScript : public ScriptInstance
{
public:
    DSPScript (sol::table tbl);
    ~DSPScript();

    void init()
    {
        if (sol::function f = DSP ["init"])
            f();
    }

    static Result validate (const String& script);

    void prepare (double rate, int block)
    {
        if (sol::function f = DSP ["prepare"])
            f (rate, block);
    }
    void release()
    {
        if (sol::function f = DSP ["release"])
            f();
    }

    void process (AudioSampleBuffer& a, MidiPipe& m);
    
    void save (MemoryBlock& block);
    void restore (const void* data, size_t size);

    const kv::PortList& getPorts() const { return ports; }
    void getPorts (kv::PortList& out);

    int getNumParameters() const { return numParams; }

    Element::Parameter::Ptr getParameterObject (int index, bool input = true) const;

    void copyParameterValues (const DSPScript&);

private:
    sol::table DSP;
    sol::function processFunc;
    AudioBuffer<float>** audio  = nullptr;
    LuaMidiPipe** midi          = nullptr;
    int processRef              = LUA_REFNIL;
    int audioRef                = LUA_REFNIL;
    int midiRef                 = LUA_REFNIL;
    lua_State* L                = nullptr;
    bool loaded                 = false;
    int numParams               = 0;
    enum { maxParams = 128 };
    float paramData [maxParams];
    sol::userdata params;
    kv::PortList ports;

    class Parameter; friend class Parameter;
    ReferenceCountedArray<Parameter> inParams, outParams;

    void deref();
    void getParameterData (MemoryBlock&);
    void setParameterData (MemoryBlock&);
    void addAudioMidiPorts();
    void addParameterPorts();
    void unlinkParams();
    void setParameter (int, float);
};

}
