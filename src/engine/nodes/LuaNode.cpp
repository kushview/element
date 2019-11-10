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

#include "engine/nodes/LuaNode.h"
#include "engine/MidiPipe.h"

static const String defaultScript = R"abc(--[[ 
    Lua Filter template

    This script came with Element and is in the public domain.

    The Lua filter node is highly experimental and the API is subject to change 
    without warning.  Please bear with us as we move toward a stable version.
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
            print (string.format ('runtime = %d seconds', seconds))
        end
    end

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

struct LuaMidiBuffer
{
    MidiBuffer* buffer;
};

struct LuaMidiPipe
{
    MidiPipe* pipe;
};

static LuaMidiPipe* lua_midi_pipe_create (lua_State* state)
{
    auto* pipe = (LuaMidiPipe*) lua_newuserdata (state, sizeof (LuaMidiPipe));
    pipe->pipe = nullptr;
    luaL_getmetatable (state, "Element.MidiPipe");
    lua_setmetatable (state, -2);
    return pipe;
}

static int lua_midi_pipe_new (lua_State* state)
{
    auto* pipe = (LuaMidiPipe*) lua_newuserdata (state, sizeof (LuaMidiPipe));
    pipe->pipe = nullptr;
    luaL_getmetatable (state, "Element.MidiPipe");
    lua_setmetatable (state, -2);
    return 1;
}

static int lua_midi_pipe_size (lua_State* state)
{
    auto* pipe = (LuaMidiPipe*) lua_touserdata (state, 1);
    lua_pushinteger (state, pipe->pipe == nullptr
        ? 0 : pipe->pipe->getNumBuffers());
    return 1;
}

static int lua_midi_pipe_to_string (lua_State *state)
{
    lua_pushstring (state, "MidiBuffer");
    return 1;
}

static const struct luaL_Reg midiPipeLib [] = {
    { "new",    lua_midi_pipe_new },
    { nullptr,  nullptr }
};

static const struct luaL_Reg midiPipeMethods [] = {
    { "__tostring", lua_midi_pipe_to_string },
    { "size", lua_midi_pipe_size },
    nullptr, nullptr
};

int luaopen_MidiPipe (lua_State* state) {
    luaL_newmetatable (state, "Element.MidiPipe");
    lua_pushvalue (state, -1);                  /* duplicate the metatable */
    lua_setfield (state, -2, "__index");        /* mt.__index = mt */
    luaL_setfuncs (state, midiPipeMethods, 0);  /* register metamethods */
    luaL_newlib (state, midiPipeLib);
    return 1;
}

struct LuaNode::Context 
{
    explicit Context () { }
    ~Context() { }

    bool ready() const { return loaded; }

    Result load (const String& script)
    {
        if (ready())
            return Result::fail ("Script already loaded");

        // load the MidiPipe module
        luaL_requiref (state, "MidiPipe", luaopen_MidiPipe, 1);
        lua_pop (state, 1);  /* remove lib */

        state.loadBuffer (script, "luanode");
        lua_pcall (state, 0, 0, 0);
        lua_getglobal (state, "name"); // -1
        name = String::fromUTF8 (lua_tostring (state, -1));
        lua_pop (state, -1);

        // store the render function
        lua_getglobal (state, "render");
        renderRef = luaL_ref (state, LUA_REGISTRYINDEX);

        // allocate a MidiPipe to reference
        lua_midi_pipe_new (state);
        midiPipeRef = luaL_ref (state, LUA_REGISTRYINDEX);

        loaded = true;
        return Result::ok();
    }

    Result validate (const String& script)
    {
        return Result::ok();
    }

    void prepare (double rate, int block)
    {
        if (state.isNull())
            return;
    
        lua_getglobal (state, "prepare");
        lua_pushnumber (state,  rate);
        lua_pushinteger (state, block);
        if (lua_pcall (state, 2, 0, 0) != LUA_OK)
        {
            DBG("[EL] error calling prepare()");
        }
    }

    void release()
    {
        if (state.isNull())
            return;
    
        lua_getglobal (state, "release"); /* function to be called */
        if (lua_pcall (state, 0, 0, 0) != LUA_OK)
        {
            DBG("[EL] error calling release()");
        }
    }

    void render (AudioSampleBuffer& audio, MidiPipe& midi)
    {
        lua_rawgeti (state, LUA_REGISTRYINDEX, renderRef);
        lua_pushnumber (state, 100);
        lua_rawgeti (state, LUA_REGISTRYINDEX, midiPipeRef);
        auto* pipe = (LuaMidiPipe*) lua_touserdata (state, -1);
        pipe->pipe = &midi;
        if (lua_pcall (state, 2, 0, 0) != LUA_OK)
        {
            // DBG("[EL] error calling render()");
        }
        pipe->pipe = nullptr;
    }

    String getName() const { return name; }

private:
    LuaState state;
    int renderRef { - 1 },
        midiPipeRef { - 1},
        audioRef { -1 };
    bool loaded { false };
    String name;
};

LuaNode::LuaNode()
    : GraphNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, EL_INTERNAL_FORMAT_NAME, nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_LUA, nullptr);
    script = draftScript = defaultScript;
    loadScript (script);
}

LuaNode::~LuaNode()
{ 
    context.reset();
}

Result LuaNode::loadScript (const String&)
{
    auto newContext = std::unique_ptr<Context> (new Context());
    auto result = newContext->load (script);
    
    if (result.wasOk())
    {
        setName (newContext->getName());
        ScopedLock sl (lock);
        context.swap (newContext);
    }

    newContext.reset();
    return result;
}

void LuaNode::fillInPluginDescription (PluginDescription& desc)
{
    desc.name               = getName();
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

void LuaNode::prepareToRender (double sampleRate, int maxBufferSize)
{
    context->prepare (sampleRate, maxBufferSize);
}

void LuaNode::releaseResources()
{
    context->release();
}

void LuaNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    ScopedLock sl (lock);
    context->render (audio, midi);
}

void LuaNode::setState (const void* data, int size)
{

}

void LuaNode::getState (MemoryBlock& block)
{

}

}
