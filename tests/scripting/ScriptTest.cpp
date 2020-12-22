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
#include "scripting/DSPScript.h"
#include "scripting/Script.h"

using namespace Element;

static const String sSyntaxError = R"(--- comment
this should trigger a syntax error
)";

static const String sExceptionError = R"(
    local obj = {}
    obj:nil_function()
)";

static const String sAnonymous = R"(
    local msg = "anon"
    testvalue = msg
    return msg
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
            beginTest ("environment");
            auto script = std::unique_ptr<Script> (new Script (lua));
            script->load (sAnonymous);
            auto env = sol::environment (lua, sol::create, lua.globals());
            env["testvalue"] = true;
            sol::object a = env["testvalue"], b = lua["testvalue"];
            expect (a != b);
            expect (a.get_type() == sol::type::boolean);
            expect (b.get_type() == sol::type::nil);

            beginTest ("environment: in lua");
            env["testvalue"] = 95.0;
            lua["testvalue"] = "hello world";
            a = env["testvalue"], b = lua["testvalue"];
            expect (a.get_type() == sol::type::number);
            expect (b.get_type() == sol::type::string);
            lua.script ("expect (testvalue == 'hello world')");
            lua.script ("expect (testvalue == 95)", env);

            sol::function call = script->caller();
            auto renv = sol::get_environment (call);
            env.set_on (call);
            renv = sol::get_environment (call);
            expect (renv.valid());
            call();
            lua.script ("expect (testvalue == 'anon')", renv);
            lua.script ("expect (testvalue == 'hello world')");
        }

        {
            beginTest ("return value");
            auto script = std::unique_ptr<Script> (new Script (lua));
            script->load (sAnonymous);
            sol::object obj = script->call();
            expect (obj.is<std::string>(), "not a string");
            expect (obj.as<std::string>() == "anon");
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
    print ("Amp.layout")
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

            beginTest ("ports");
            const auto& ports = dsp.getPorts();
            expect (ports.size (kv::PortType::Audio,   true)  == 2);
            expect (ports.size (kv::PortType::Audio,   false) == 2);
            expect (ports.size (kv::PortType::Midi,    true)  == 0);
            expect (ports.size (kv::PortType::Midi,    false) == 0);
            expect (ports.size (kv::PortType::Control, true)  == 1);
            expect (ports.size (kv::PortType::Control, false) == 0);

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
