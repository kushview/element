/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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
#include "engine/NodeObject.h"

namespace Element {

class LuaNode : public NodeObject,
                public ChangeBroadcaster
{
public:
    using Ptr = ReferenceCountedObjectPtr<LuaNode>;

    explicit LuaNode() noexcept;
    virtual ~LuaNode();
    
    struct Context;

    void getPluginDescription (PluginDescription& desc) const override;
    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void setState (const void* data, int size) override;
    void getState (MemoryBlock& block) override;
    
    Result loadScript (const String&);

    const String& getScript() const { return script; }
    const String& getDraftScript() const { return draftScript; }
    void setDraftScript (const String& draft) { draftScript = draft; }
    bool hasChanges() const { return script.hashCode64() != draftScript.hashCode64(); }

    /** Set a parameter value by index
     
        @param index    The parameter index to set
        @param value    The value to set
    */
    void setParameter (int index, float value);

protected:
    inline bool wantsMidiPipe() const override { return true; }
    void createPorts() override;
    Parameter::Ptr getParameter (const PortDescription& port) override;

private:
    String script, draftScript;
    int blockSize = 512;
    double sampleRate = 44100.0;
    bool prepared = false;
    CriticalSection lock;
    std::unique_ptr<Context> context;
    ParameterArray inParams, outParams;
};

}
