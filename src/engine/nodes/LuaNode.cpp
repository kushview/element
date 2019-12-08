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

#include <math.h>
#include "sol/sol.hpp"
#include "lrt/lrt.h"

#include "ElementApp.h"
#include "engine/nodes/LuaNode.h"
#include "engine/MidiPipe.h"
#include "engine/Parameter.h"
#include "scripting/LuaBindings.h"

#define EL_LUA_DBG(x)
// #define EL_LUA_DBG(x) DBG(x)

static const String stereoAmpScript = 
R"(
--- Stereo Amplifier in Lua
--
-- This script came with Element and is in the public domain.
--
-- The code contained implements a simple stereo amplifier plugin.
-- It does not try to smooth the volume parameter and could cause
-- zipper noise.
--
-- The Lua filter node is highly experimental and the API is subject 
-- to change without warning.  Please bear with us as we move toward 
-- a stable version. If you are a developer and want to help out, 
-- see https://github.com/kushview/element

-- Track last applied gain value
local last_gain = 1.0

-- Return a table of audio/midi inputs and outputs
function node_io_ports()
   return {
      audio_ins   = 2,
      audio_outs  = 2,
      midi_ins    = 0,
      midi_outs   = 0
   }
end

-- Return parameters
function node_params()
   return {
      {
         name    = "Volume",
         label   = "dB",
         type    = "float",
         flow    = "input",
         min     = -90.0,
         max     = 24.0,
         default = 0.0
      }
   }
end

--- Prepare for rendering
--  Allocate any special data needed here
function node_prepare (rate, block)
   -- nothing to do in this example
end

--- Render audio and midi
--  Use the provided audio and midi objects to process your plugin
function node_render (details)
   local gain = audio.dbtogain (Param.values[1])
   
   --[[
   -- fade from last gain to new gain
   audio:apply_gain_ramp (0, audio.nframes, last_gain, gain)
   -- update last gain value
   last_gain = gain;
   midi:clear()
   --]]
end

--- Release node resources
--  Free any allocated resources in this callback
function node_release()
end

--- Save node state
--
--  This is an optional function you can implement to save state.  
--  The host will prepare the IO stream so all you have to do is 
--  `io.write(...)` your data
--
--  Note: Parameter values will automatically be saved and restored,
--  you do not need to handle them here.
function node_save()
   io.write("some custom state data")
end

--- Restore node state
--  This is an optional function you can implement to restore state.  
--  The host will prepare the IO stream so all you have to do is 
--  `io.read(...)` your data previsouly written in `node_save()`
function node_restore()
   print ("restored data:")
   print (io.read ("*a"));
end
)";

namespace Element {

struct LAudioBuffer
{
    AudioSampleBuffer* buffer;
    bool isref;
};

int audiobuffer_new (lua_State* L) {
    auto* obj = (LAudioBuffer*) lua_newuserdata (L, sizeof (LAudioBuffer));
    obj->buffer = new AudioSampleBuffer();
    obj->isref = false;
    luaL_getmetatable (L, "Element.AudioBuffer");
    lua_setmetatable (L, -2);
    return 1;
}

static int audiobuffer_gc (lua_State* L) {
    auto* obj = (LAudioBuffer*) lua_touserdata (L, 1);
    if (! obj->isref)
        delete obj->buffer;
    obj->buffer = nullptr;
    return 1;
}

static int audiobuffer_tostring (lua_State* L) {
    lua_pushstring (L, "AudioBuffer");
    return 1;
}

static int audiobuffer_length (lua_State* L) {
    auto* obj = (LAudioBuffer*) lua_touserdata (L, 1);
    lua_pushinteger (L, obj->buffer->getNumSamples());
    return 1;
}

static int audiobuffer_channels (lua_State* L) {
    auto* obj = (LAudioBuffer*) lua_touserdata (L, 1);
    lua_pushinteger (L, obj->buffer->getNumChannels());
    return 1;
}

static int audiobuffer_resize (lua_State* L) {
    auto* obj = (LAudioBuffer*) lua_touserdata (L, 1);
    // obj->buffer->setSize()
    return 0;
}

static luaL_Reg audiobuffer_f[] = {
    { "__call",    audiobuffer_new },
    { "new",       audiobuffer_new },
    { nullptr,  nullptr }
};

static luaL_Reg audiobuffer_m[] = {
    { "__gc",       audiobuffer_gc },
    { "__tostring", audiobuffer_tostring },
    { "length",     audiobuffer_length },
    { "channels",   audiobuffer_channels },
    { "resize",     audiobuffer_resize },
    { nullptr, nullptr }
};

int luaopen_audiobuffer (lua_State* L) {
    DBG("stack = " << lua_gettop(L));
    luaL_newmetatable (L, "Element.AudioBuffer");
    DBG("stack = " << lua_gettop(L));
    lua_pushvalue (L, -1);                  /* duplicate the metatable */
    DBG("stack = " << lua_gettop(L));
    lua_setfield (L, -2, "__index");        /* mt.__index = mt */
    DBG("stack = " << lua_gettop(L));
    luaL_setfuncs (L, audiobuffer_m, 0);
    DBG("stack = " << lua_gettop(L));
    luaL_newlib (L, audiobuffer_f);
    DBG("stack = " << lua_gettop(L));
    return 1;
}

//=============================================================================
class RenderDetails
{
public:
    constexpr static const char* metatable = "Element.RenderDetails";

    RenderDetails()
    {
        dummyAudio.setSize (1, 512, false, true, false);
        reset();
    }

    ~RenderDetails() { DBG ("~RenderDetails()"); }

    void reset()
    {
        audio = &dummyAudio;
        midi  = &dummyMidi;
    }
    
    int getNumSamples() const { return 0; }

    static int open (lua_State* L)
    {
        static luaL_Reg classMethods[] = {
            { "new",    RenderDetails::construct },
            { nullptr,  nullptr }
        };

        static luaL_Reg instanceMethods[] = {
            { "__gc",       RenderDetails::destroy },
            { "__tostring", RenderDetails::tostring },
            { "nframes",    RenderDetails::nframes },
            { "nchannels",  RenderDetails::nchannels },
            { "nmidi",      RenderDetails::nmidi },
            { "sample",     RenderDetails::sample },
            { nullptr, nullptr }
        };

        luaL_newmetatable (L, metatable);
        lua_pushvalue (L, -1);  /* duplicate the metatable */
        lua_setfield (L, -2, "__index");  /* mt.__index = mt */
        luaL_setfuncs (L, instanceMethods, 0);
        luaL_newlib (L, classMethods);
        return 1;
    }

    static int construct (lua_State* L)
    {
        auto** details = (RenderDetails**) lua_newuserdata (L, sizeof (RenderDetails*));
        *details = new RenderDetails();
        luaL_getmetatable (L, metatable);
        lua_setmetatable (L, -2);
        return 1;
    }

    static int destroy (lua_State* L)
    {
        auto** details = (RenderDetails**) lua_touserdata (L, 1);
        if (*details != nullptr)
            deleteAndZero (*details);
        return 1;
    }

    static int sample (lua_State* L)
    {
        auto* details = get (L);
        auto s = details->audio->getSample (0, 0);
        lua_pushnumber (L, static_cast<lua_Number> (s));
        return 1;
    }

    static int tostring (lua_State* L)
    {
        auto* details = get (L);
        String str = "RenderDetails: ";
        str << " nframes=" << details->audio->getNumSamples() <<
               " nchannels=" << details->audio->getNumChannels() <<
               " nmidi=" << details->midi->getNumBuffers();
        lua_pushstring (L, str.toRawUTF8());
        return 1;
    }

    static int nframes (lua_State* L)
    {
        auto* details = get (L);
        lua_pushinteger (L, details->audio->getNumSamples());
        return 1;
    }

    static int nchannels (lua_State* L)
    {
        auto* details = get (L);
        lua_pushinteger (L, details->audio->getNumChannels());
        return 1;
    }

    static int nmidi (lua_State* L)
    {
        auto* details = get (L);
        lua_pushinteger (L, details->midi->getNumBuffers());
        return 1;
    }

    static RenderDetails* get (lua_State* L, int index = 1)
    {
        return *static_cast<RenderDetails**> (lua_touserdata (L, index));
    }

private:
    friend class LuaNode::Context;
    AudioSampleBuffer* audio { nullptr };
    MidiPipe* midi { nullptr };
    AudioSampleBuffer dummyAudio;
    MidiPipe dummyMidi;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RenderDetails)
};

//=============================================================================
class LuaParameter : public ControlPortParameter,
                     public Parameter::Listener
{
public:
    LuaParameter (LuaNode::Context* c, const PortDescription& port)
        : ControlPortParameter (port),
          ctx (c)
    {
        const auto sp = getPort();
        set (sp.defaultValue);
        addListener (this);
    }

    ~LuaParameter()
    {
        unlink();
    }

    String getLabel() const override { return {}; }

    void unlink()
    {
        removeListener (this);
        ctx = nullptr;
    }

    void controlValueChanged (int parameterIndex, float newValue) override;
    void controlTouched (int parameterIndex, bool gestureIsStarting) override;
    
private:
    LuaNode::Context* ctx { nullptr };
};

//=============================================================================
struct LuaNode::Context
{
    explicit Context () { }
    ~Context()
    {
        for (auto* ip : inParams)
            dynamic_cast<LuaParameter*>(ip)->unlink();
        for (auto* op : outParams)
            dynamic_cast<LuaParameter*>(op)->unlink();
        inParams.clear();
        outParams.clear();

        luaL_unref (state, LUA_REGISTRYINDEX, renderRef);
        luaL_unref (state, LUA_REGISTRYINDEX, detailsRef);
        state.collect_garbage();
    }

    String getName() const { return name; }

    bool ready() const { return loaded; }

    Result load (const String& script)
    {
        if (ready())
            return Result::fail ("Script already loaded");

        String errorMsg;
        try
        {
            state.open_libraries (sol::lib::base, sol::lib::string,
                                  sol::lib::io, sol::lib::package);
            Lua::registerEngine (state);
            auto res = state.script (script.toRawUTF8());
            
            if (res.valid())
            {
                // renderf = state ["node_render"];
                // renderstdf = renderf;

                bool ok = false;
                if (lua_getglobal (state, "node_render") == LUA_TFUNCTION)
                {
                    renderRef = luaL_ref (state, LUA_REGISTRYINDEX);
                    ok = renderRef != LUA_REFNIL && renderRef != LUA_NOREF;
                }

                if (ok)
                {
                    audioBuffer = lrt_audio_buffer_new (state, 0, 0);
                    audioBufRef = luaL_ref (state, LUA_REGISTRYINDEX);
                    ok = audioBufRef != LUA_REFNIL && audioBufRef != LUA_NOREF;
                }

                if (ok)
                {
                    midiPipe = lrt_midi_pipe_new (state, 0);
                    midiPipeRef = luaL_ref (state, LUA_REGISTRYINDEX);
                    ok = midiPipeRef != LUA_REFNIL && midiPipeRef != LUA_NOREF;
                }
                
                loaded = ok;
            }
        }
        catch (const std::exception& e)
        {
            errorMsg = e.what();
            loaded = false;
            renderstdf = nullptr;
        }

        if (loaded)
        {
            addIOPorts();
            addParameters();
            auto param = state["Param"].get_or_create<sol::table>();
            param["values"] = &paramData;
        }
        else
        {
            ports.clear();
        }

        return loaded ? Result::ok()
            : Result::fail (errorMsg.isNotEmpty() 
                ? errorMsg : String("unknown error in script"));
    }

    static Result validate (const String& script)
    {
        if (script.isEmpty())
            return Result::fail ("script contains no code");

        auto ctx = std::make_unique<Context>();
        auto result = ctx->load (script);
        if (result.failed())
            return result;
        if (! ctx->ready())
            return Result::fail ("could not parse script");

        try
        {
            const int block = 1024;
            const double rate = 44100.0;

            using PT = kv::PortType;
            
            // call node_io_ports() and node_params()
            PortList validatePorts;
            ctx->getPorts (validatePorts);

            // create a dummy audio buffer and midipipe
            auto nchans = jmax (validatePorts.size (PT::Audio, true),
                                validatePorts.size (PT::Audio, false));
            auto nmidi  = jmax (validatePorts.size (PT::Midi, true),
                                validatePorts.size (PT::Midi, false));
            AudioSampleBuffer audio (jmax (1, nchans), block);
            OwnedArray<MidiBuffer> midiBufs;
            Array<int> midiIdx;
            
            while (midiBufs.size() < nmidi)
            {
                midiIdx.add (midiBufs.size());
                midiBufs.add (new MidiBuffer ());
            }

            // calls node_prepare(), node_render(), and node_release()
            {
                auto midi = midiBufs.size() > 0 
                    ? std::make_unique<MidiPipe> (midiBufs, midiIdx)
                    : std::make_unique<MidiPipe>();
                ctx->prepare (rate, block);
                
                // user renderf directly so it can throw an exception
                // if (ctx->renderf)
                //     ctx->renderf (std::ref (audio), std::ref (*midi));
                ctx->render (audio, *midi);
                ctx->release();
            }

            midiBufs.clearQuick (true);
            ctx.reset();
            result = Result::ok();
        }
        catch (const std::exception& e)
        {
            result = Result::fail (e.what());
        }

        return result;
    }

    void prepare (double rate, int block)
    {
        if (! ready())
            return;

        if (auto fn = state ["node_prepare"])
            fn (rate, block);
        
        state.collect_garbage();
    }

    void release()
    {
        if (! ready())
            return;

        if (auto fn = state ["node_release"])
            fn();
        
        state.collect_garbage();
    }

    void render (AudioSampleBuffer& audio, MidiPipe& midi) noexcept
    {
        if (! loaded)
            return;

        if (lua_rawgeti (state, LUA_REGISTRYINDEX, renderRef) == LUA_TFUNCTION)
        {
            if (lua_rawgeti (state, LUA_REGISTRYINDEX, audioBufRef) == LUA_TUSERDATA)
            {
                lrt_audio_buffer_duplicate_32 (audioBuffer, audio.getArrayOfReadPointers(),
                                                            audio.getNumChannels(),
                                                            audio.getNumSamples());
                if (lua_rawgeti (state, LUA_REGISTRYINDEX, midiPipeRef) == LUA_TUSERDATA)
                {
                    lua_call (state, 2, 0);
                }
            }
        }
        else
        {
            DBG("didn't get render fucntion in callback");
        }
    }

    const OwnedArray<PortDescription>& getPortArray() const noexcept
    {
        return ports.getPorts();
    }

    void getPorts (PortList& results)
    {
        for (const auto* port : ports.getPorts())
            results.add (new PortDescription (*port));
    }

    Parameter* getParameter (const PortDescription& port)
    {
        auto& params = port.input ? inParams : outParams;
        auto param = params [port.channel];
        jassert (param != nullptr);
        return param;
    }

    void setParameter (int index, float value) noexcept
    {
        jassert (isPositiveAndBelow (index, maxParams));
        paramData[index] = value;
    }

    void getParameterData (MemoryBlock& block) const
    {
        block.append (paramData, sizeof(float) * (size_t) inParams.size());
    }

    void setParameterData (const MemoryBlock& block)
    {
        jassert (block.getSize() % sizeof(float) == 0);
        jassert (block.getSize() < sizeof(float) * maxParams);
        memcpy (paramData, block.getData(), block.getSize());
        for (int i = 0; i < inParams.size(); ++i)
            if (auto* param = dynamic_cast<LuaParameter*> (inParams.getObjectPointerUnchecked (i)))
                param->set (paramData [i]);
    }

    void copyParameterValues (const Context& other)
    {
        for (int i = jmin (inParams.size(), other.inParams.size()); --i >= 0;)
            paramData[i] = other.paramData[i];

        for (auto* const ip : inParams)
        {
            auto* const param = dynamic_cast<LuaParameter*> (ip);
            const auto  port  = param->getPort();
            paramData[port.channel] = jlimit (port.minValue, port.maxValue, paramData[port.channel]);
            param->setValue (param->convertTo0to1 (paramData[port.channel]));
        }
    }

    void getState (MemoryBlock& block)
    {
        sol::function save = state ["node_save"];
        if (! save.valid())
            return;
        
        auto result = state.safe_script(R"(
            local tf = io.tmpfile()
            local oo = io.output()
            io.output (tf);
            node_save()
            tf:seek ('set', 0)
            local data = tf:read ("*a")
            io.close()
            io.output (oo);
            return data
        )");

        if (result.valid())
        {
            sol::object data = result;
            if (data.is<const char*>())
            {
                //DBG("data = " << data.as<const char*>());
                MemoryOutputStream mo (block, false);
                mo.write (data.as<const char*>(), strlen (data.as<const char*>()));
            }
        }
    }

    void setState (const void* data, size_t size)
    {
        sol::function restore = state["node_restore"];
        if (! restore.valid())
            return;

        sol::userdata ud = state.script ("return io.tmpfile()");
        FILE* const file = ud.as<FILE*>(); // Lua will close this
        fwrite (data, 1, size, file);

        state["__state_data__"] = ud;
        state.safe_script (R"(
            local oi = io.input()
            __state_data__:seek ('set', 0)
            io.input (__state_data__)
            node_restore()
            print (io.read ("*a"))
            io.input(oi)
            __state_data__:close()
            __state_data__ = nil
        )");

        state["__state_data__"] = nullptr;
        state.collect_garbage();
    }

private:
    sol::state state;
    sol::function renderf;
    std::function<void(AudioSampleBuffer&, MidiPipe&)> renderstdf;
    String name;
    bool loaded = false;

    int renderRef  = LUA_NOREF;
    int detailsRef = LUA_NOREF;
    int audioBufRef = LUA_NOREF;
    int midiPipeRef = LUA_NOREF;
    lrt_midi_pipe_t* midiPipe { nullptr };
    lrt_audio_buffer_t* audioBuffer { nullptr };

    PortList ports;
    ParameterArray inParams, outParams;

    int numParams = 0;
    enum { maxParams = 512 };
    float paramData [maxParams];
    float paramDataOut [maxParams];

    LuaParameter* findParameter (const PortDescription& port) const
    {
        for (auto* const ip : inParams)
        {
            auto* const param = dynamic_cast<LuaParameter*> (ip);
            const auto port = param->getPort();
            ignoreUnused (param, port);
        }

        return nullptr;
    }

    void resetParameters() noexcept
    {
        memset (paramData, 0, sizeof(float) * (size_t) maxParams);
    }

    void addParameters()
    {
        if (auto f = state ["node_params"])
        {
            int index = ports.size();
            int inChan = 0, outChan = 0;

            try
            {
                sol::table params = f();
                for (int i = 0; i < params.size(); ++i)
                {
                    auto param = params [i + 1];

                    String name  = param["name"].get_or (std::string ("Param"));
                    String sym   = name.trim().toLowerCase().replace(" ", "_");
                    String type  = param["type"].get_or (std::string ("float"));
                    String flow  = param["flow"].get_or (std::string ("input"));
                    jassert (flow == "input" || flow == "output");
                    
                    bool isInput = flow == "input";
                    float min    = param["min"].get_or (0.0);
                    float max    = param["max"].get_or (1.0);
                    float dfault = param["default"].get_or (1.0);
                    ignoreUnused (min, max, dfault);
                    const int channel = isInput ? inChan++ : outChan++;

                    EL_LUA_DBG("index = " << index);
                    EL_LUA_DBG("channel = " << channel);
                    EL_LUA_DBG("is input = " << (int) isInput);
                    EL_LUA_DBG("name = " << name);
                    EL_LUA_DBG("symbol = " << sym);
                    EL_LUA_DBG("min = " << min);
                    EL_LUA_DBG("max = " << max);
                    EL_LUA_DBG("default = " << dfault);

                    if (isInput)
                    {
                        paramData[channel] = dfault;
                    }

                    ports.addControl (index++, channel, sym, name,
                                      min, max, dfault, isInput);
                    auto& params = isInput ? inParams : outParams;
                    params.add (new LuaParameter (this, ports.getPort (ports.size() - 1)));
                }

                numParams = ports.size (PortType::Control, true);
            }
            catch (const std::exception&)
            {

            }
        }
    }

    void addIOPorts()
    {
        auto& lua = state;

        if (auto f = lua ["node_io_ports"])
        {
            sol::table t = f();
            int audioIns = 0, audioOuts = 0,
                midiIns = 0, midiOuts = 0;

            try {
                if (t.size() == 0)
                {
                    audioIns  = t["audio_ins"].get_or (0);
                    audioOuts = t["audio_outs"].get_or (0);
                    midiIns   = t["midi_ins"].get_or (0);
                    midiOuts  = t["midi_outs"].get_or (0);
                }
                else
                {
                    audioIns  = t[1]["audio_ins"].get_or (0);
                    audioOuts = t[1]["audio_outs"].get_or (0);
                    midiIns   = t[1]["midi_ins"].get_or (0);
                    midiOuts  = t[1]["midi_outs"].get_or (0);
                }
            }
            catch (const std::exception&) {}

            int index = 0, channel = 0;
            for (int i = 0; i < audioIns; ++i)
            {
                String slug = "in_"; slug << (i + 1);
                String name = "In "; name << (i + 1);
                ports.add (PortType::Audio, index++, channel++,
                           slug, name, true);
            }

            channel = 0;
            for (int i = 0; i < audioOuts; ++i)
            {
                String slug = "out_"; slug << (i + 1);
                String name = "Out "; name << (i + 1);
                ports.add (PortType::Audio, index++, channel++,
                           slug, name, false);
            }

            channel = 0;
            for (int i = 0; i < midiIns; ++i)
            {
                String slug = "midi_in_"; slug << (i + 1);
                String name = "MIDI In "; name << (i + 1);
                ports.add (PortType::Midi, index++, channel++,
                           slug, name, true);
            }

            channel = 0;
            for (int i = 0; i < midiOuts; ++i)
            {
                String slug = "midi_out_"; slug << (i + 1);
                String name = "MIDI Out "; name << (i + 1);
                ports.add (PortType::Midi, index++, channel++,
                           slug, name, false);
            }
        }
    }
};

void LuaParameter::controlValueChanged (int index, float value)
{
    if (ctx != nullptr) // index may not be set so use port channel.
        ctx->setParameter (getPortChannel(), convertFrom0to1 (value));
}

void LuaParameter::controlTouched (int, bool) {}

LuaNode::LuaNode() noexcept
    : GraphNode (0)
{
    context = std::make_unique<Context>();
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, EL_INTERNAL_FORMAT_NAME, nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_LUA, nullptr);
    loadScript (stereoAmpScript);
}

LuaNode::~LuaNode()
{
    context.reset();
}

void LuaNode::createPorts()
{
    if (context == nullptr)
        return;
    ports.clearQuick();
    context->getPorts (ports);
}

Parameter::Ptr LuaNode::getParameter (const PortDescription& port)
{
    return context->getParameter (port);
}

Result LuaNode::loadScript (const String& newScript)
{
    auto result = Context::validate (newScript);
    if (result.failed())
        return result;
    
    auto newContext = std::make_unique<Context>();
    result = newContext->load (newScript);

    if (result.wasOk())
    {
        script = draftScript = newScript;
        if (prepared)
            newContext->prepare (sampleRate, blockSize);
        triggerPortReset();
        ScopedLock sl (lock);
        if (context != nullptr)
            newContext->copyParameterValues (*context);
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
    const auto state = ValueTree::readFromGZIPData (data, size);
    if (state.isValid())
    {
        // May want to do this procedure async with a Message::post()
        auto result = loadScript (state["script"].toString());

        if (result.wasOk())
        {
            if (state.hasProperty ("params"))
            {
                const var& params = state.getProperty ("params");
                if (params.isBinaryData())
                    if (auto* block = params.getBinaryData())
                        context->setParameterData (*block);
            }

            if (state.hasProperty ("data"))
            {
                const var& data = state.getProperty ("data");
                if (data.isBinaryData())
                    if (auto* block = data.getBinaryData())
                        context->setState (block->getData(), block->getSize());
            }
        }
        sendChangeMessage();
    }
}

void LuaNode::getState (MemoryBlock& block)
{
    ValueTree state ("LuaNodeState");
    state.setProperty ("script", script, nullptr)
         .setProperty ("draft",  draftScript, nullptr);

    MemoryBlock scriptBlock;
    context->getParameterData (scriptBlock);
    if (scriptBlock.getSize() > 0)
        state.setProperty ("params", scriptBlock, nullptr);

    scriptBlock.reset();
    context->getState (scriptBlock);
    if (scriptBlock.getSize() > 0)
        state.setProperty ("data", scriptBlock, nullptr);

    MemoryOutputStream mo (block, false);
    {
        GZIPCompressorOutputStream gz (mo);
        state.writeToStream (gz);
    }
}

void LuaNode::setParameter (int index, float value)
{
    ScopedLock sl (lock);
    context->setParameter (index, value);
}

}
