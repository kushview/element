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

#include "engine/nodes/BaseProcessor.h"
#include "engine/GraphNode.h"
#include "sol/sol.hpp"

namespace Element {

class DSPScript;

class ScriptNode : public GraphNode,
                   public ChangeBroadcaster
{
public:
    using Ptr = ReferenceCountedObjectPtr<ScriptNode>;

    explicit ScriptNode() noexcept;
    virtual ~ScriptNode();

    struct Context;

    void fillInPluginDescription (PluginDescription& desc);
    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void setState (const void* data, int size) override;
    void getState (MemoryBlock& block) override;

    Result loadScript (const String&);

    CodeDocument& getCodeDocument (bool forEditor = false) { return forEditor ? edCode : dspCode; }

    /** Set a parameter value by index
     
        @param index    The parameter index to set
        @param value    The value to set
    */
    void setParameter (int index, float value);

    lua_State* getLuaState() const { return lua.lua_state(); }

protected:
    inline bool wantsMidiPipe() const override { return true; }
    void createPorts() override;
    Parameter::Ptr getParameter (const PortDescription& port) override;

private:
    CriticalSection lock;
    sol::state lua;
    CodeDocument dspCode, edCode;
    std::unique_ptr<DSPScript> script;
    ParameterArray inParams, outParams;

    int blockSize = 512;
    double sampleRate = 44100.0;
    bool prepared = false;
};

}
