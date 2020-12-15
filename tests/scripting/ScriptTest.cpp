/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "LuaUnitTest.h"
#include "scripting/Script.h"
#include "kv/lua/factories.hpp"

using namespace Element;

static const String sSyntaxError = R"(--- comment
this should trigger a syntax error
)";

static const String sExceptionError = R"(
    local obj = {}
    obj:nil_function()
)";


//=============================================================================
class ScriptTest : public LuaUnitTest
{
public:
    ScriptTest()
        : LuaUnitTest ("Script", "Script", "basics") {}
    
    void runTest() override
    {
        const auto ampFile = File::getCurrentWorkingDirectory()
            .getChildFile ("scripts/amp.lua");
        {
            beginTest ("load");
            auto script = std::unique_ptr<Script> (new Script (lua));
            expect (! script->isLoaded(), String ("Should not be loaded"));
            expect (! script->hasError());
            script->load (ampFile);
            expect (script->isLoaded(), "Should be loaded");
            beginTest ("script name");
            expect (script->getName().toLowerCase() == "amp", script->getName());
            beginTest ("script type");
            expect (script->getType() == "DSP", script->getType());
            beginTest ("script author");
            expect (script->getAuthor() == "Michael Fisher", script->getAuthor());
            beginTest ("call type");
            auto amp = script->call();
            expect (amp.valid() && amp.get_type() == sol::type::table);
        }

        {
            auto script = std::unique_ptr<Script> (new Script (lua, ampFile));
            expect (script->isLoaded() && !script->hasError(), script->getErrorMessage());
        }

        {
            auto script = std::unique_ptr<Script> (new Script (lua, sSyntaxError));
            beginTest (String ("load with error"));
            expect (script->hasError(), script->getErrorMessage());
            DBG(script->getErrorMessage());

            beginTest ("error");
            auto result = script->call();
            expect (! result.valid(), sol::type_name (lua, result.get_type()));
            expect (script->getErrorMessage().containsIgnoreCase ("error"));
            DBG (script->getErrorMessage());
        }
        
        {
            auto script = std::unique_ptr<Script> (new Script (lua));
            script->load (sExceptionError);
            script->call();
            beginTest (String("exception: ") + script->getErrorMessage());
            expect (script->getErrorMessage().isNotEmpty());
        }

        {
            beginTest ("base64 encode");
            String urlStr = "base64://"; urlStr << Util::toBase64 (sExceptionError);
            URL url (urlStr);
            const auto decoded = Util::fromBase64 (url.toString(false).replace ("base64://", ""));
            expect (decoded.trim() == sExceptionError.trim(), decoded);
        }

        {
            beginTest ("gzip encode/decode");
            String urlStr = "gzip://"; urlStr << gzip::encode (sExceptionError);
            URL url (urlStr);
            const auto decompressed = gzip::decode (url.toString(false).replace ("gzip://", ""));
            expect (sExceptionError.trim() == decompressed.trim());
        }
    }
};

static ScriptTest sScriptTest;

//=============================================================================
const static String sAmp = R"(
local Amp = {}
local audio  = require ('kv.audio')

local gain1 = 1.0
local gain2 = 1.0
local statedata = "some save data from save/restore"

Amp.rate = 0
Amp.block = 0
Amp.released = false
Amp.initialized = false

function Amp.init()
    gain1 = 0.0
    gain2 = 0.0
    Amp.initialized = true
end

function Amp.layout()
    return {
        audio = { 2, 2 },
        midi  = { 0, 0 }
    }
end

function Amp.params()
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

function Amp.prepare (r, b)
    print ("Amp.prepare", r, b)
    begintest ("correct rate and block")
    expect (r == 44100)
    expect (b == 4096)
    Amp.rate = r
    Amp.block = b
end

function Amp.process (a, m)
   a:fade (gain1, gain2)
end

function Amp.release()
    Amp.released = true
end

function Amp.save()
   io.write (statedata)
end

function Amp.restore()
   begintest ("read saved data")
   expect (io.read ("a") == statedata);
end

return Amp

)";

class DSPScript
{
public:
    DSPScript (sol::table tbl)
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

        loaded = ok;
        if (! loaded)
        {
            derefAudioMidi();
        }
    }

    ~DSPScript()
    {
        derefAudioMidi();
    }

    void init()
    {
        if (sol::function f = DSP ["init"])
            f();
    }

    void prepare (double rate, int block)
    {
        if (sol::function f = DSP ["prepare"])
            f (rate, block);
    }

    void release()
    {
        if (sol::function f = DSP ["release"])
            f();
    }

    void process (AudioSampleBuffer& a, MidiPipe& m)
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

    void save (MemoryBlock& block)
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

    void restore (const void* data, size_t size)
    {
        sol::function restore = DSP["restore"];
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

private:
    sol::table DSP;
    sol::function processFunc;
    AudioBuffer<float>** audio  = nullptr;
    LuaMidiPipe** midi          = nullptr;
    int processRef              = LUA_REFNIL;
    int audioRef                = LUA_REFNIL;
    int midiRef                 = LUA_REFNIL;
    lua_State* L                = nullptr;
    bool loaded                 = false;

    void derefAudioMidi()
    {
        loaded = false;
        audio = nullptr;
        luaL_unref (DSP.lua_state(), LUA_REGISTRYINDEX, audioRef);
        audioRef = LUA_REFNIL;
        midi = nullptr;
        luaL_unref (DSP.lua_state(), LUA_REGISTRYINDEX, midiRef);
        midiRef = LUA_REFNIL;
    }
};

//=============================================================================
class DSPScriptTest : public LuaUnitTest
{
public:
    DSPScriptTest()
        : LuaUnitTest ("DSP Script", "Script", "DSP") { }

    void runTest() override
    {
        {
            beginTest ("load");
            auto script = std::unique_ptr<Script> (new Script (lua));
            script->load (sAmp);
            if (script->hasError())
            {
                expect (false, String ("Could not load script: ") + script->getErrorMessage());
                return;
            }

            expect (! script->hasError(), script->getErrorMessage());
            if (script->hasError())
                return;
            
            auto result = script->call();
            expect(result.get_type() == sol::type::table, sol::type_name (result.lua_state(), result.get_type()));
            sol::table Amp = result;
            DSPScript dsp (Amp);

            beginTest ("init");
            dsp.init();
            expect (Amp.get_or ("initialized", false), "didn't call init");

            dsp.prepare (44100, 4096);
            beginTest ("prepared");
            expect (Amp.get_or ("rate", 0.0) == 44100.0, String (Amp.get_or ("rate", 0)));
            expect (Amp.get_or ("block", 0.0) == 4096.0, String (Amp.get_or ("block", 0)));

            beginTest ("processed");
            AudioSampleBuffer audio (2, 4096);
            for (int c = 0; c < 2; ++c)
                for (int f = 0; f < 4096; ++f)
                    audio.setSample (c, f, 1.0);
            MidiPipe midi;
            dsp.process (audio, midi);

            for (int c = 0; c < 2; ++c)
                for (int f = 0; f < 4096; ++f)
                    if (audio.getSample (c, f) < -0.00001 || audio.getSample (c, f) > 0.00001)
                        { expect (false, "bad rendering"); return; }

            MemoryBlock block;
            beginTest ("save");
            dsp.save (block);

            beginTest ("restore");
            dsp.restore (block.getData(), block.getSize());

            beginTest ("release");
            expect (Amp.get_or ("released", true) == false);
            dsp.release();
            expect (Amp.get_or ("released", false) == true);
        }

        lua.collect_garbage();
    }
};

static DSPScriptTest sDSPScriptTest;
