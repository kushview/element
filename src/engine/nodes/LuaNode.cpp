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

-- prepare for rendering
function prepare(rate, block)
    print (string.format ('prepare rate = %d block = %d', rate, block))
end

-- render audio and midi
function render (audio, midi)
    -- render the streams
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

LuaNode::LuaNode()
    : GraphNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, EL_INTERNAL_FORMAT_NAME, nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_LUA, nullptr);

    script = draftScript = defaultScript;

    state.loadBuffer (script, "luanode");
    lua_pcall (state, 0, 0, 0);
    lua_getglobal (state, "name"); // -1
    setName (lua_tostring (state, -1));
    lua_pop (state, -1);
    lua_getglobal (state, "render");
    renderRef = luaL_ref (state, LUA_REGISTRYINDEX);
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
    lua_pushnumber (state, 101);
    if (lua_pcall (state, 2, 0, 0) != LUA_OK)
    {
        DBG("[EL] error calling render()");
    }
}

void LuaNode::setState (const void* data, int size)
{

}

void LuaNode::getState (MemoryBlock& block)
{

}

}
