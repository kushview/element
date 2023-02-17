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

#include "el/factories.hpp"
#include <element/midipipe.hpp>
#include "scripting/dspscript.hpp"

using namespace element;
namespace element {

//==============================================================================
class DSPScript::Parameter : public ControlPortParameter,
                             public element::Parameter::Listener
{
public:
    Parameter (DSPScript* c, const PortDescription& port)
        : ControlPortParameter (port),
          ctx (c)
    {
        const auto sp = getPort();
        set (sp.defaultValue);
        addListener (this);
    }

    ~Parameter() override
    {
        unlink();
    }

    String getLabel() const override { return {}; }

    void unlink()
    {
        removeListener (this);
        ctx = nullptr;
    }

    void controlValueChanged (int index, float value) override
    {
        if (ctx != nullptr) // index may not be set so use port channel.
            ctx->setParameter (getPortChannel(), convertFrom0to1 (value));
    }

    void controlTouched (int parameterIndex, bool gestureIsStarting) override {}

    void update (float value)
    {
        removeListener (this);
        set (value);
        addListener (this);
    }

private:
    DSPScript* ctx { nullptr };
};

//==============================================================================
DSPScript::DSPScript (sol::table tbl)
    : DSP (tbl)
{
    bool ok = DSP.valid();
    if (ok)
    {
        L = DSP.lua_state();
        ok = L != nullptr;
    }

    if (ok)
    {
        try
        {
            sol::state_view lua (L);
            auto result = lua.safe_script (R"(
                require ('el.audio')
                require ('el.midi')
                require ('el.AudioBuffer')
                require ('el.MidiBuffer')
                require ('el.MidiMessage')
                require ('el.MidiPipe')
            )");
            ok = result.status() == sol::call_status::ok;

            switch (result.status())
            {
                case sol::call_status::file:
                    DBG ("DSPScript: file error");
                    break;
                case sol::call_status::gc:
                    DBG ("DSPScript: gc error");
                    break;
                case sol::call_status::handler:
                    DBG ("DSPScript: handler error");
                    break;
                case sol::call_status::memory:
                    DBG ("DSPScript: memory error");
                    break;
                case sol::call_status::runtime:
                    DBG ("DSPScript: runtime error");
                    break;
                case sol::call_status::syntax:
                    DBG ("DSPScript: syntax error");
                    break;
                case sol::call_status::yielded:
                    DBG ("DSPScript: yielded error");
                    break;
                case sol::call_status::ok:
                    break;
            }
        } catch (const sol::error& e)
        {
            DBG (e.what());
            ok = false;
        }
    }

    if (ok)
    {
        processFunc = DSP["process"];
        processRef = processFunc.registry_index();
        ok = processRef != LUA_REFNIL && processRef != LUA_NOREF;
    }

    if (ok)
    {
        audio = lua::new_userdata<AudioBuffer<float>> (
            L, EL_MT_AUDIO_BUFFER_32);
        audioRef = luaL_ref (L, LUA_REGISTRYINDEX);
        ok = audioRef != LUA_REFNIL && audioRef != LUA_NOREF;
    }

    if (ok)
    {
        midi = LuaMidiPipe::create (L, 4);
        midiRef = luaL_ref (L, LUA_REGISTRYINDEX);
        ok = midiRef != LUA_REFNIL && midiRef != LUA_NOREF;
    }

    if (ok)
    {
        addAudioMidiPorts();
        addParameterPorts();
    }

    if (ok)
    {
        sol::state_view view (L);
        auto tmp = view.create_table();
        tmp["params"] = &paramData;
        params = tmp["params"];
        ok = params.valid();
    }

    loaded = ok;
    if (! loaded)
    {
        deref();
    }
}

DSPScript::~DSPScript()
{
    unlinkParams();
    deref();
}

Result DSPScript::validate (const String& script)
{
    if (script.isEmpty())
        return Result::fail ("script contains no code");
    return Result::ok();
#if 0
    Script loader;
    loader.load (script);

    auto result = ctx->load (script);
    if (result.failed())
        return result;
    
    auto ctx = std::make_unique<DSPScript>();
    if (! ctx->ready())
        return Result::fail ("could not parse script");

    try
    {
        const int block = 1024;
        const double rate = 44100.0;

        using PT = PortType;
        
        // call node_io_ports() and node_params()
        PortList validatePorts;
        ctx->getPorts (validatePorts);

        // create a dummy audio buffer and midipipe
        auto nchans = jmax (validatePorts.size (PT::Audio, true),
                            validatePorts.size (PT::Audio, false));
        auto nmidi  = jmax (validatePorts.size (PT::Midi, true),
                            validatePorts.size (PT::Midi, false));
        
        ctx->prepare (rate, block);

        ctx->state["__ln_validate_rate"]    = rate;
        ctx->state["__ln_validate_nmidi"]   = nmidi;
        ctx->state["__ln_validate_nchans"]  = nchans;
        ctx->state["__ln_validate_nframes"] = block;
        ctx->state.script (R"(
            function __ln_validate_render()
                local AudioBuffer = require ('el.AudioBuffer')
                local MidiPipe    = require ('el.MidiPipe')

                local a = AudioBuffer (__ln_validate_nchans, __ln_validate_nframes)
                local m = MidiPipe (__ln_validate_nmidi)
                
                for _ = 1,4 do
                    for i = 0,m:size() - 1 do
                        local b = m:get(i)
                        b:insert (0, midi.noteon (1, 60, math.random (1, 127)))
                        b:insert (10, midi.noteoff (1, 60, 0))
                    end
                    node_render (a, m)
                    a:clear()
                    m:clear()
                end
                
                a = nil
                m = nil
                collectgarbage()
            end

            __ln_validate_render()
            __ln_validate_render = nil
            collectgarbage()
        )");

        ctx->release();
        ctx.reset();
        result = Result::ok();
    }
    catch (const std::exception& e)
    {
        result = Result::fail (e.what());
    }
    return result;
#endif
}

void DSPScript::getPorts (PortList& out)
{
    for (const auto* port : ports.getPorts())
        out.add (new PortDescription (*port));
}

void DSPScript::process (AudioSampleBuffer& a, MidiPipe& m)
{
    if (! loaded)
        return;

    if (lua_rawgeti (L, LUA_REGISTRYINDEX, processRef) == LUA_TFUNCTION)
    {
        if (lua_rawgeti (L, LUA_REGISTRYINDEX, audioRef) == LUA_TUSERDATA)
        {
            if (lua_rawgeti (L, LUA_REGISTRYINDEX, midiRef) == LUA_TUSERDATA)
            {
                if (lua_rawgeti (L, LUA_REGISTRYINDEX, params.registry_index()) == LUA_TUSERDATA)
                {
                    (*audio)->setDataToReferTo (a.getArrayOfWritePointers(),
                                                a.getNumChannels(),
                                                a.getNumSamples());
                    (*midi)->swapWith (m);

                    lua_call (L, 3, 0);

                    (*midi)->swapWith (m);
                }
            }
        }
    }
    else
    {
        DBG ("didn't get render function in callback");
    }
}

void DSPScript::save (MemoryBlock& out)
{
    ValueTree state ("DSP");
    MemoryBlock block;

    block.reset();
    getParameterData (block);
    if (block.getSize() > 0)
        state.setProperty ("params", block, nullptr);

    sol::function save = DSP["save"];

    if (save.valid())
    {
        sol::state_view lua (L);
        sol::environment env (lua, sol::create, lua.globals());
        try
        {
            env["dsp_script_save"] = save;
            auto result = lua.safe_script (R"(
                local tf = io.tmpfile()
                local oo = io.output()
                io.output (tf);
                dsp_script_save()
                tf:seek ('set', 0)
                local data = tf:read ("*a")
                io.close()
                io.output (oo);
                dsp_script_save = nil
                return data
            )",
                                           env);

            if (result.valid())
            {
                sol::object data = result;
                if (data.is<const char*>())
                {
                    block.reset();
                    MemoryOutputStream mo (block, false);
                    mo.write (data.as<const char*>(), strlen (data.as<const char*>()));
                }
            }
        } catch (const std::exception& e)
        {
            DBG ("[EL] " << e.what());
        }
        lua.collect_garbage();
    }

    MemoryOutputStream mo (out, false);
    {
        GZIPCompressorOutputStream gz (mo);
        state.writeToStream (gz);
    }
}

void DSPScript::restore (const void* d, size_t s)
{
    const auto state = ValueTree::readFromGZIPData (d, s);
    if (! state.isValid())
        return;

    const var& params = state.getProperty ("params");
    if (params.isBinaryData())
    {
        setParameterData (*params.getBinaryData());
        for (auto* const param : inParams)
        {
            const auto port = param->getPort();
            param->update (paramData[port.channel]);
        }
    }

    const var& data = state.getProperty ("data");
    sol::function restore = DSP["restore"];
    if (! restore.valid() || ! data.isBinaryData())
        return;

    try
    {
        sol::state_view lua (L);
        sol::environment env (lua, sol::create, lua.globals());
        sol::userdata ud = lua["io"]["tmpfile"]();
        luaL_Stream* const stream = (luaL_Stream*) ud.pointer();

        fwrite (data.getBinaryData()->getData(), 1, data.getBinaryData()->getSize(), stream->f);
        rewind (stream->f);

        env["__state_data__"] = ud;
        env["dsp_script_restore"] = restore;
        lua.safe_script (R"(
            local oi = io.input()
            io.input (__state_data__)
            dsp_script_restore()
            io.input (oi)
            __state_data__:close()
            __state_data__ = nil
            dsp_script_restore = nil
        )",
                         env);

        lua.collect_garbage();
    } catch (const std::exception& e)
    {
        DBG ("[EL] " << e.what());
    }
}

void DSPScript::setParameter (int index, float value)
{
    paramData[index] = value;
}

void DSPScript::copyParameterValues (const DSPScript& o)
{
    for (int i = jmin (numParams, o.numParams); --i >= 0;)
        paramData[i] = o.paramData[i];
}

void DSPScript::getParameterData (MemoryBlock& block)
{
    block.append (paramData, sizeof (float) * static_cast<size_t> (numParams));
}

void DSPScript::setParameterData (MemoryBlock& block)
{
    jassert (block.getSize() % sizeof (float) == 0);
    jassert (block.getSize() < sizeof (float) * maxParams);
    memcpy (paramData, block.getData(), block.getSize());
}

String DSPScript::getUI() const
{
    if (! DSP.valid())
        return {};
    if (DSP["ui"].get_type() == sol::type::string)
        return DSP["ui"].get<std::string>();
    return {};
}

void DSPScript::deref()
{
    loaded = false;
    audio = nullptr;
    luaL_unref (L, LUA_REGISTRYINDEX, audioRef);
    audioRef = LUA_REFNIL;
    midi = nullptr;
    luaL_unref (L, LUA_REGISTRYINDEX, midiRef);
    midiRef = LUA_REFNIL;
}

void DSPScript::addAudioMidiPorts()
{
    sol::function f = DSP["layout"];
    if (! f.valid())
        return;

    try
    {
        int numAudioIn = 0, numAudioOut = 0,
            numMidiIn = 0, numMidiOut = 0;

        sol::table layout = f();
        if (layout.size() > 0)
            layout = layout[1];

        sol::table audio = layout["audio"].get_or_create<sol::table>();
        numAudioIn = audio[1].get_or (0);
        numAudioOut = audio[2].get_or (0);
        sol::table midi = layout["midi"].get_or_create<sol::table>();
        numMidiIn = midi[1].get_or (0);
        numMidiOut = midi[2].get_or (0);

        int index = ports.size();
        int channel = 0;

        for (int i = 0; i < numAudioIn; ++i)
        {
            String slug = "in_";
            slug << (i + 1);
            String name = "In ";
            name << (i + 1);
            ports.add (PortType::Audio, index++, channel++, slug, name, true);
        }

        channel = 0;
        for (int i = 0; i < numAudioOut; ++i)
        {
            String slug = "out_";
            slug << (i + 1);
            String name = "Out ";
            name << (i + 1);
            ports.add (PortType::Audio, index++, channel++, slug, name, false);
        }

        channel = 0;
        for (int i = 0; i < numMidiIn; ++i)
        {
            String slug = "midi_in_";
            slug << (i + 1);
            String name = "MIDI In ";
            name << (i + 1);
            ports.add (PortType::Midi, index++, channel++, slug, name, true);
        }

        channel = 0;
        for (int i = 0; i < numMidiOut; ++i)
        {
            String slug = "midi_out_";
            slug << (i + 1);
            String name = "MIDI Out ";
            name << (i + 1);
            ports.add (PortType::Midi, index++, channel++, slug, name, false);
        }
    } catch (const std::exception&)
    {
    }
}

element::Parameter::Ptr
    DSPScript::getParameterObject (int index, bool input) const
{
    return input ? inParams[index] : outParams[index];
}

void DSPScript::addParameterPorts()
{
    sol::function f = DSP["parameters"];
    if (! f.valid())
        return;

    try
    {
        int index = ports.size();
        int inChan = 0, outChan = 0;
        sol::table params = f();
        for (size_t i = 0; i < params.size(); ++i)
        {
            auto param = params[i + 1];

            String name = param["name"].get_or (std::string ("Param ") + String (i + 1).toStdString());
            String sym = param["symbol"].get_or (std::string());
            if (sym.isEmpty())
            {
                sym = name.trim().toLowerCase().replaceCharacter ('-', '_').replaceCharacter (' ', '_');
            }

            String type = param["type"].get_or (std::string ("float"));
            String flow = param["flow"].get_or (std::string ("input"));
            jassert (flow == "input" || flow == "output");

            bool isInput = flow == "input";
            float min = param["min"].get_or (0.0);
            float max = param["max"].get_or (1.0);
            float dfault = param["default"].get_or (1.0);
            ignoreUnused (min, max, dfault);
            const int channel = isInput ? inChan++ : outChan++;

            // EL_LUA_DBG("index = " << index);
            // EL_LUA_DBG("channel = " << channel);
            // EL_LUA_DBG("is input = " << (int) isInput);
            // EL_LUA_DBG("name = " << name);
            // EL_LUA_DBG("symbol = " << sym);
            // EL_LUA_DBG("min = " << min);
            // EL_LUA_DBG("max = " << max);
            // EL_LUA_DBG("default = " << dfault);

            if (isInput)
            {
                paramData[channel] = dfault;
            }

            ports.addControl (index++, channel, sym, name, min, max, dfault, isInput);
        }

        numParams = ports.size (PortType::Control, true);

        unlinkParams();
        for (const auto* port : ports.getPorts())
        {
            if (port->type == PortType::Control && port->input)
                inParams.add (new Parameter (this, *port));
            else if (port->type == PortType::Control && ! port->input)
                outParams.add (new Parameter (this, *port));
        }
    } catch (const std::exception&)
    {
    }
}

void DSPScript::unlinkParams()
{
    for (auto* p : inParams)
        p->unlink();
    for (auto* p : outParams)
        p->unlink();

    inParams.clearQuick();
    outParams.clearQuick();
}

} // namespace element