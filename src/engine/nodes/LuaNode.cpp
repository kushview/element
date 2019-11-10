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

static const String defaultScript = R"%(--[[ 
    Lua Filter template

    This script came with Element and is in the public domain.

    The Lua filter node is highly experimental and the API is subject to change 
    without warning.  Please bear with us as we move toward a stable version.
--]]

-- The name of the node
name = "Lua Filter"

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
sample_rate = 0
frame_count = 0
block_size = 0

-- prepare for rendering
function prepare(rate, block)
    sample_rate = rate
    frame_count = 0
    print (string.format ('prepare rate = %d block = %d', rate, block))
end

-- render audio and midi
function render (audio, midi)
    -- render the streams
    print (midi)
    if frame_count % sampe_rate == 0 then
        print (frame_count)
        print (midi)
    end

    --- why doesn't this increment??
    frame_count = frame_count + 1
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

)%";

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

LuaNode::LuaNode()
    : GraphNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, EL_INTERNAL_FORMAT_NAME, nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_LUA, nullptr);

    script = draftScript = defaultScript;

    luaL_requiref (state, "MidiPipe", luaopen_MidiPipe, 1);
    lua_pop (state, 1);  /* remove lib */

    state.loadBuffer (script, "luanode");
    lua_pcall (state, 0, 0, 0);
    lua_getglobal (state, "name"); // -1
    setName (lua_tostring (state, -1));
    lua_pop (state, -1);

    // get the render function
    lua_getglobal (state, "render");
    renderRef = luaL_ref (state, LUA_REGISTRYINDEX);

    DBG("Stack Size: " << lua_gettop (state));

    lua_midi_pipe_new (state);
    // if (luaMidiPipe == nullptr)
    {
        DBG("Stack Size: " << lua_gettop (state));
        midiPipeRef = luaL_ref (state, LUA_REGISTRYINDEX);
    }
    
    // luaMidiPipe = (LuaMidiPipe*) lua_newuserdata (state, sizeof (LuaMidiPipe));
    // luaMidiPipe->pipe = nullptr;
    // midiPipeRef = luaL_ref (state, LUA_REGISTRYINDEX);
}

LuaNode::~LuaNode()
{ 

}

void LuaNode::fillInPluginDescription (PluginDescription& desc)
{
    desc.name               = "Lua Filter";
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
    if (state.isNull())
        return;
    
    lua_getglobal  (state, "prepare");
    lua_pushnumber (state, sampleRate);
    lua_pushnumber (state, maxBufferSize);
    if (lua_pcall (state, 2, 0, 0) != LUA_OK)
    {
        DBG("[EL] error calling prepare()");
    }
}

void LuaNode::releaseResources()
{
    if (state.isNull())
        return;
    
    lua_getglobal (state, "release"); /* function to be called */
    if (lua_pcall (state, 0, 0, 0) != LUA_OK)
    {
        DBG("[EL] error calling release()");
    }
}

void LuaNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    lua_rawgeti (state, LUA_REGISTRYINDEX, renderRef);
    lua_pushnumber (state, 100);
//    lua_pushnumber (state, 101);
    lua_rawgeti (state, LUA_REGISTRYINDEX, midiPipeRef);
    if (lua_pcall (state, 2, 0, 0) != LUA_OK)
    {
        // DBG("[EL] error calling render()");
    }
}

void LuaNode::setState (const void* data, int size)
{

}

void LuaNode::getState (MemoryBlock& block)
{

}

}
