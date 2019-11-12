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

#include "sol/sol.hpp"
#include "scripting/LuaBindings.h"
#include "engine/nodes/LuaNode.h"
#include "engine/MidiPipe.h"

static const String defaultScript = R"abc(--[[ 
    Lua template

    This script came with Element and is in the public domain.

    The Lua filter node is highly experimental and the API is subject to change 
    without warning.  Please bear with us as we move toward a stable version. If
    you are a developer and want to help out, see https://github.com/kushview/element
--]]

-- The name of the node
name = "Lua"

-- Port descriptions
ports = {
    {
        index = 0,
        channel = 0,
        type = "midi",
        input = true,
        name = "MIDI In",
        slug = "midi_in"
    },
    {
        index = 1,
        channel = 0,
        type = "midi",
        input = false,
        name = "MIDI Out",
        slug = "midi_out"
    }
}

-- throttler
local sample_rate = 0
local frame_count = 0
local block_size = 0
local cylces_done = 0

-- prepare for rendering
function prepare (rate, block)
    sample_rate = rate
    frame_count = 0
    block_size = block
    cycles_done = 0 
    print (string.format ('prepare rate = %d block = %d', rate, block))
end

-- render audio and midi
function render (audio, midi)
    local r = sample_rate
    local b = frame_count
    local e = frame_count + block_size - 1

    for i = b, e do
        local f = i
        if f % r == 0 then
            local seconds = f / r
            print (string.format ("size = %i", midi.size))
            print (string.format ('runtime = %d seconds', seconds))
            if midi:num_buffers() > 0 then
                buffer = midi:read_buffer(0)
                -- for m in buffer.data do print(m) end
            end
        end
    end

    midi:clear()
    frame_count = e
end

-- release resources
function release()
    print('release resources...')
end

-- save state
function save(memory)
end

-- restore state
function restore(memory)
end

)abc";

namespace Element {

struct LuaNode::Context 
{
    explicit Context () { }
    ~Context() { }

    bool ready() const { return loaded; }

    Result load (const String& script)
    {
        if (ready())
            return Result::fail ("Script already loaded");
        
        try {
            state.open_libraries();
            Lua::registerEngine (state);
            state.script (script.toStdString());
            renderf = state ["render"];
            loaded = true;
        } catch (const std::exception& e) {
            Logger::writeToLog (e.what());
            loaded = false;
        }

        return loaded ? Result::ok() : Result::fail ("Couldn't load Lua script");
    }

    Result validate (const String& script)
    {
        return Result::ok();
    }

    void prepare (double rate, int block)
    {
        if (! ready())
            return;

        if (auto fn = state ["prepare"])
            fn (rate, block);
    }

    void release()
    {
        if (! ready())
            return;

        if (auto fn = state ["release"])
            fn();
    }

    void render (AudioSampleBuffer& audio, MidiPipe& midi)
    {
        if (loaded)
            renderf (audio, midi);

        // lua_rawgeti (state, LUA_REGISTRYINDEX, renderRef);
        // lua_pushnumber (state, 100);
        // lua_rawgeti (state, LUA_REGISTRYINDEX, midiPipeRef);
        // auto* pipe = (LuaMidiPipe*) lua_touserdata (state, -1);
        // pipe->pipe = &midi;
        // if (lua_pcall (state, 2, 0, 0) != LUA_OK)
        // {
        //     // DBG("[EL] error calling render()");
        // }
        // pipe->pipe = nullptr;
    }

    String getName() const { return name; }

private:
    sol::state state;
    sol::function renderf;
    String name;
    bool loaded = false;
};

LuaNode::LuaNode()
    : GraphNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, EL_INTERNAL_FORMAT_NAME, nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_LUA, nullptr);
    loadScript (defaultScript);
}

LuaNode::~LuaNode()
{ 
    context.reset();
}

Result LuaNode::loadScript (const String& newScript)
{
    auto newContext = std::unique_ptr<Context> (new Context());
    auto result = newContext->load (newScript);
    
    if (result.wasOk())
    {
        script = draftScript = newScript;
        if (prepared)
            newContext->prepare (sampleRate, blockSize);
        ScopedLock sl (lock);
        context.swap (newContext);
    }

    if (newContext != nullptr)
    {
        newContext->release();
        newContext.reset();
    }

    return result;
}

void LuaNode::fillInPluginDescription (PluginDescription& desc)
{
    desc.name               = "Lua";
    desc.fileOrIdentifier   = EL_INTERNAL_ID_LUA;
    desc.uid                = EL_INTERNAL_UID_LUA;
    desc.descriptiveName    = "A user scriptable Element node";
    desc.numInputChannels   = 0;
    desc.numOutputChannels  = 0;
    desc.hasSharedContainer = false;
    desc.isInstrument       = false;
    desc.manufacturerName   = "Element";
    desc.pluginFormatName   = EL_INTERNAL_FORMAT_NAME;
    desc.version            = "1.0.0";
}

void LuaNode::prepareToRender (double rate, int block)
{
    if (prepared)
        return;
    sampleRate = rate;
    blockSize = block;
    context->prepare (sampleRate, blockSize);
    prepared = true;
}

void LuaNode::releaseResources()
{
    if (! prepared)
        return;
    prepared = false;
    context->release();
}

void LuaNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    ScopedLock sl (lock);
    context->render (audio, midi);
}

void LuaNode::setState (const void* data, int size)
{
    const auto state = ValueTree::readFromData (data, size);
    if (state.isValid())
    {
        loadScript (state["script"].toString());
        sendChangeMessage();
    }
}

void LuaNode::getState (MemoryBlock& block)
{
    ValueTree state ("lua");
    state.setProperty ("script", script, nullptr)
         .setProperty ("draft", draftScript, nullptr);
    MemoryOutputStream mo (block, false);
    state.writeToStream (mo);
}

}
