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

static const String defaultScript = R"(--[[
    Lua template

    This script came with Element and is in the public domain.

    The Lua filter node is highly experimental and the API is subject to change
    without warning.  Please bear with us as we move toward a stable version. If
    you are a developer and want to help out, see https://github.com/kushview/element
--]]

-- prepare for rendering
function prepare (rate, block)
    print (string.format ('prepare rate = %d block = %d', rate, block))
end

function MidiBuffer:messages()
    local iter = MidiBufferIterator.make (self)
    local msg = MidiMessage.make()
    return function()
        local ok, frame = iter:get_next_event (msg)
        if not ok then
            return nil
        else
            return msg, frame
        end
    end
end

-- render audio and midi
function render (audio, midi)
    audio:clear()
    local midi_buffer = midi:get_read_buffer (0)
    for msg, _ in midi_buffer:messages() do
        print (msg)
    end
    midi_buffer:clear()
end

-- release resources
function release()
    print('release resources...')
end

)";

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
            state.open_libraries (sol::lib::base, sol::lib::string);
            Lua::registerEngine (state);
            state.script (script.toRawUTF8());
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
        if (! loaded)
            return;

       #if 1
        /*
            TODO: don't call renderf directly.  Figure out how to cash metatable
            indexes for the passed types and use lua C api directly.  this object
            probably allocates and/or has several other function calls all of which
            take time See below... If we can figure out how to push refs to audio/midi +
            a direct way to specify the metatable type, then it should greatly
            improve performance.
        */
        renderf (audio, midi);
       #else
        lua_rawgeti (state, LUA_REGISTRYINDEX, renderf.registry_index());
        lua_pushlightuserdata (state, &audio);
        luaL_setmetatable (state, "AudioBuffer");
        lua_pushlightuserdata (state, &midi);
        luaL_setmetatable (state, "MidiPipe");
        if (lua_pcall (state, 2, 0, 0) != LUA_OK)
        {

        }
       #endif
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
    context = std::make_unique<Context>();
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
