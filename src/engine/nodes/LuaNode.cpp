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
#include "ElementApp.h"
#include "engine/nodes/LuaNode.h"
#include "engine/MidiPipe.h"
#include "engine/Parameter.h"
#include "scripting/LuaBindings.h"

#define EL_LUA_DBG(x)
// #define EL_LUA_DBG(x) DBG(x)

static const String defaultScript = 
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

--- prepare for rendering
function node_prepare (rate, block)
   -- nothing to do in this example
end

-- render audio and midi
function node_render (audio, midi)
   local nframes = audio:get_num_samples()
   local gain = decibels.to_gain (Param.values[1])

   -- fade from last gain to new gain
   audio:apply_gain_ramp (0, nframes, last_gain, gain)
    
   -- update last gain value
   last_gain = gain;
   midi:clear()
end

--- Release node resources
--  free any allocated resources in this callback
function node_release()
end

--- Save node state
--  This is an optional function you can implement to save state.  
--  The host will prepare the IO stream so all you have to do is 
--  `io.write(...)` your data
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

class LuaParameter : public ControlPortParameter,
                     public Parameter::Listener
{
public:
    LuaParameter (LuaNode::Context* c, const PortDescription& port)
        : ControlPortParameter (port),
          ctx (c)
    { 
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

    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;
    
private:
    LuaNode::Context* ctx { nullptr };
};

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
            state.open_libraries (sol::lib::base, sol::lib::string, sol::lib::io);
            Lua::registerEngine (state);
            auto res = state.script (script.toRawUTF8());
            if (res.valid())
            {
                renderf = state ["node_render"];
                renderstdf = renderf;
                loaded = true;
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
                if (ctx->renderf)
                    ctx->renderf (std::ref (audio), std::ref (*midi));

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
        if (loaded)
            renderstdf (audio, midi);
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
            tf:seek ("set", 0)
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
                {
                    // GZIPCompressorOutputStream gz (mo);
                    mo.write (data.as<const char*>(), strlen (data.as<const char*>()));
                }
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
            __state_data__:seek ("set", 0)
            io.input (__state_data__)
            node_restore()
            print (io.read("*a"))
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

void LuaParameter::parameterValueChanged (int index, float value)
{
    if (ctx != nullptr)
        ctx->setParameter (index, convertFrom0to1 (value));
}

void LuaParameter::parameterGestureChanged (int, bool) {}

LuaNode::LuaNode() noexcept
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
    const auto state = ValueTree::readFromData (data, size);
    if (state.isValid())
    {
        auto result = loadScript (state["script"].toString());
        if (result.wasOk() && state.hasProperty ("data"))
        {
            const var& data = state.getProperty ("data");
            if (data.isBinaryData())
                if (auto* block = data.getBinaryData())
                    context->setState (block->getData(), block->getSize());
        }
        sendChangeMessage();
    }
}

void LuaNode::getState (MemoryBlock& block)
{
    ValueTree state ("lua");
    state.setProperty ("script", script, nullptr)
         .setProperty ("draft", draftScript, nullptr);

    MemoryBlock scriptBlock;
    context->getState (scriptBlock);
    if (scriptBlock.getSize() > 0)
        state.setProperty ("data", scriptBlock, nullptr);

    MemoryOutputStream mo (block, false);
    state.writeToStream (mo);
}

void LuaNode::setParameter (int index, float value)
{
    ScopedLock sl (lock);
    context->setParameter (index, value);
}

}
