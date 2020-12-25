
#include "kv/lua/factories.hpp"
#include "engine/MidiPipe.h"
#include "scripting/DSPScript.h"

using namespace kv;
namespace Element {

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
        sol::state_view lua (L);
        ok = lua.safe_script (R"(
            require ('kv.audio')
            require ('kv.midi')
            require ('kv.AudioBuffer')
            require ('kv.MidiBuffer')
            require ('kv.MidiMessage')
            require ('el.MidiPipe')
        )").status() == sol::call_status::ok;
    }

    if (ok)
    {
        processFunc = DSP ["process"];
        processRef = processFunc.registry_index();
        ok = processRef != LUA_REFNIL && processRef != LUA_NOREF;
    }

    if (ok)
    {
        audio = kv::lua::new_userdata<AudioBuffer<float>> (
            L, LKV_MT_AUDIO_BUFFER_32);
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

    loaded = ok;
    if (! loaded)
    {
        derefAudioMidi();
    }
}

DSPScript::~DSPScript()
{
    derefAudioMidi();
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

        using PT = kv::PortType;
        
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
                local AudioBuffer = require ('kv.AudioBuffer')
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

    const auto nchans  = a.getNumChannels();
    const auto nframes = a.getNumSamples();
    const auto nmidi   = m.getNumBuffers();

    if (lua_rawgeti (L, LUA_REGISTRYINDEX, processRef) == LUA_TFUNCTION)
    {
        if (lua_rawgeti (L, LUA_REGISTRYINDEX, audioRef) == LUA_TUSERDATA)
        {
            if (lua_rawgeti (L, LUA_REGISTRYINDEX, midiRef) == LUA_TUSERDATA)
            {
                (*audio)->setDataToReferTo (a.getArrayOfWritePointers(),
                        a.getNumChannels(), a.getNumSamples());
                (*midi)->swapWith (m);

                lua_call (L, 2, 0);
                
                (*midi)->swapWith (m);
            }
        }
    }
    else
    {
        DBG("didn't get render fucntion in callback");
    }
}

void DSPScript::save (MemoryBlock& block)
{
    sol::function save = DSP ["save"];
    if (! save.valid())
        return;
    
    try {
        sol::state_view lua (L);
        sol::environment env (lua, sol::create, lua.globals());
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
        )", env);

        if (result.valid())
        {
            sol::object data = result;
            if (data.is<const char*>())
            {
                MemoryOutputStream mo (block, false);
                mo.write (data.as<const char*>(), strlen (data.as<const char*>()));
            }
        }

        lua.collect_garbage();
    } catch (const std::exception& e) {
        DBG("[EL] " << e.what());
    }
}

void DSPScript::restore (const void* data, size_t size)
{
    sol::function restore = DSP ["restore"];
    if (! restore.valid())
        return;

    try {
        sol::state_view lua (L);
        sol::environment env (lua, sol::create, lua.globals());
        sol::userdata ud = lua["io"]["tmpfile"]();
        luaL_Stream* const stream = (luaL_Stream*) ud.pointer();

        fwrite (data, 1, size, stream->f);
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
        )", env);

        lua.collect_garbage();
    } catch (const std::exception& e) { 
        DBG("[EL] " << e.what());
    }
}

void DSPScript::derefAudioMidi()
{
    loaded = false;
    audio = nullptr;
    luaL_unref (DSP.lua_state(), LUA_REGISTRYINDEX, audioRef);
    audioRef = LUA_REFNIL;
    midi = nullptr;
    luaL_unref (DSP.lua_state(), LUA_REGISTRYINDEX, midiRef);
    midiRef = LUA_REFNIL;
}

void DSPScript::addAudioMidiPorts()
{
    sol::function f = DSP ["layout"];
    if (! f.valid())
        return;

    try {
        int numAudioIn = 0, numAudioOut = 0,
            numMidiIn = 0, numMidiOut = 0;
        
        sol::table layout = f();
        if (layout.size() > 0)
            layout = layout [1];

        sol::table audio    = layout["audio"].get_or_create<sol::table>();
        numAudioIn          = audio[1].get_or (0);
        numAudioOut         = audio[2].get_or (0);
        sol::table midi     = layout["midi"].get_or_create<sol::table>();
        numMidiIn           = midi[1].get_or (0);
        numMidiOut          = midi[2].get_or (0);

        int index = ports.size();
        int channel = 0;
        for (int i = 0; i < numAudioIn; ++i)
        {
            String slug = "in_"; slug << (i + 1);
            String name = "In "; name << (i + 1);
            ports.add (PortType::Audio, index++, channel++,
                    slug, name, true);
        }

        channel = 0;
        for (int i = 0; i < numAudioOut; ++i)
        {
            String slug = "out_"; slug << (i + 1);
            String name = "Out "; name << (i + 1);
            ports.add (PortType::Audio, index++, channel++,
                    slug, name, false);
        }

        channel = 0;
        for (int i = 0; i < numMidiIn; ++i)
        {
            String slug = "midi_in_"; slug << (i + 1);
            String name = "MIDI In "; name << (i + 1);
            ports.add (PortType::Midi, index++, channel++,
                    slug, name, true);
        }

        channel = 0;
        for (int i = 0; i < numMidiOut; ++i)
        {
            String slug = "midi_out_"; slug << (i + 1);
            String name = "MIDI Out "; name << (i + 1);
            ports.add (PortType::Midi, index++, channel++,
                    slug, name, false);
        }
    } catch (const std::exception&) {

    }
}

void DSPScript::addParameterPorts()
{
    sol::function f = DSP ["params"];
    if (! f.valid()) return;

    try {
        int index = ports.size();
        int inChan = 0, outChan = 0;
        sol::table params = f();
        for (size_t i = 0; i < params.size(); ++i)
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

            ports.addControl (index++, channel, sym, name,
                                min, max, dfault, isInput);
        }

        numParams = ports.size (PortType::Control, true);
    }
    catch (const std::exception&) {}
}
}