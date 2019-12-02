/*
    This file is part of Element
    Copyright (C) 2018-2019  Kushview, LLC.  All rights reserved.

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

#include "Tests.h"

#if 1

#include "engine/nodes/LuaNode.h"
#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

using namespace Element;

static const String nodeScript = R"(
if element and element.plugin then
    element.plugin ({
        type        = "node",
        name        = "Test Node",
        author      = "Michael Fisher",
        description = [[ Test script registration ]]
    })
end

function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1,
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

prepared_flag = false

function node_prepare (sample_rate, block_size)
    print ("prepare test node")
    prepared_flag = true
end

function node_render (details)
    local d = RenderDetails.new()
    print(d)
    print(d:nframes())
    print(d:nchannels())
    print(d:sample())
end

function node_release()
    prepared_flag = false
end
)";

static const String globalSyntaxError = R"(

bad global syntax in script

function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1,
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

prepared_flag = false

function node_prepare (sample_rate, block_size)
    print ("prepare test node")
    prepared_flag = true
end

function node_render (details)
    print (details)
end

function node_release()
    prepared_flag = false
end
)";

static const String funcSyntaxError = R"(
function node_io_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1,
        midi_outs   = 0
    }
    return --- can't do this in Lua
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

prepared_flag = false

function node_prepare (sample_rate, block_size)
    print ("prepare test node")
    prepared_flag = true
end

function node_render (details)
    print(details)
end

function node_release()
    prepared_flag = false
end
)";

static const String accessNilObject = R"(

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
    return {}
end

prepared_flag = false

function node_prepare (sample_rate, block_size)
    prepared_flag = true
end

function node_render (details)
    print(details)
end

function node_release()
end
)";

//=============================================================================
class LuaNodeLifecycleTest : public UnitTestBase
{
public:
    LuaNodeLifecycleTest() : UnitTestBase ("Lua Node Lifecycle", "LuaNode", "lifecycle") {}
    virtual ~LuaNodeLifecycleTest() { }
    void initialise() override
    {
        lua.open_libraries();
        Element::Lua::registerEngine (lua);
    }

    void runTest() override
    {
        auto graph = std::make_unique<GraphProcessor>();
        graph->prepareToPlay (44100.0, 1024);
        auto* node = new LuaNode();
        
        beginTest ("validate");
        auto result = node->loadScript (nodeScript);
        expect (result.wasOk());
        if (! result.wasOk())
            return;

        beginTest ("prepare");
        graph->addNode (node);

        beginTest ("ports");
        expect (node->getNumPorts() == 6);
        expect (node->getNumPorts (kv::PortType::Audio,   true)  == 2);
        expect (node->getNumPorts (kv::PortType::Audio,   false) == 2);
        expect (node->getNumPorts (kv::PortType::Midi,    true)  == 1);
        expect (node->getNumPorts (kv::PortType::Midi,    false) == 0);
        expect (node->getNumPorts (kv::PortType::Control, true)  == 1);
        
        beginTest ("release");
        graph->releaseResources();

        node = nullptr;
        graph = nullptr;
    }

private:
    sol::state lua;
};

static LuaNodeLifecycleTest sLuaNodeLifecycleTest;

//=============================================================================
class LuaNodeValidateTest : public UnitTestBase
{
public:
    LuaNodeValidateTest() : UnitTestBase ("Lua Node Validation", "LuaNode", "validate") { }
    virtual ~LuaNodeValidateTest() { }
    void initialise() override { }

    void runTest() override
    {
        auto node = std::make_unique<LuaNode>();

        beginTest ("validation ok");
        auto result = node->loadScript (nodeScript);
        expect (result.wasOk());
        return;

        beginTest ("global syntax error");
        result = node->loadScript (globalSyntaxError);
        expect (result.failed());

        beginTest ("runtime syntax error");
        node = std::make_unique<LuaNode>();
        result = node->loadScript (funcSyntaxError);
        expect (result.failed());

        beginTest ("access nil midi buffer");
        node = std::make_unique<LuaNode>();
        result = node->loadScript (accessNilObject);
        expect (result.failed());

        beginTest ("no code");
        node = std::make_unique<LuaNode>();
        result = node->loadScript ({});
        expect (result.failed());
    }

private:
    sol::state lua;
};

static LuaNodeValidateTest sLuaNodeValidateTest;

class StaticMethodTest : public UnitTestBase
{
public:
    StaticMethodTest() : UnitTestBase ("Lua Static Method", "Lua", "static") {}
    virtual ~StaticMethodTest() { }
    void initialise() override
    {
        lua.open_libraries();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        beginTest ("static method");
        lua.new_usertype<Object> ("Object", sol::no_constructor,
            "one_hundred", Object::oneHundred
        );

        try {
            lua.script (R"(
                Object.two_hundred = function() return 200 end
                value100 = Object:one_hundred()
                value200 = Object:two_hundred()
            )");
            expect (100 == (int) lua["value100"]);
            expect (200 == (int) lua["value200"]);
        } catch (const std::exception& e) {
            expect (false, e.what());
        }
    }

private:
    struct Object { inline static int oneHundred() { return 100; } };
    sol::state lua;
};

static StaticMethodTest sStaticMethodTest;


class CArrayTest : public UnitTestBase
{
public:
    CArrayTest() : UnitTestBase ("Lua C-Array", "Lua", "carray") {}
    virtual ~CArrayTest() { }
    void initialise() override
    {
        lua.open_libraries();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        beginTest ("raw");
        int size = 100;
       
        values.reset (new float [size]);
        // auto rawValues = new float [size];
        float rawValues [100];
        std::vector<float> vvalues (100, 0);
        vvalues.reserve (size);
        for (int i = 0; i < 100; ++i)
        {
            rawValues[i] = (float) i + 100.f;
            vvalues[i] = rawValues[i];
        }

        lua["params"] = std::ref(rawValues); //[this]() { return &rawValues; };
        lua["size"] = [&size]() -> int { return size; };
        size = 10;
        
        try {
            lua.script (R"(
                for k = 1, size() do
                    v = params[k]
                    print(" ", k, v)
                end
            )");
        } catch (const std::exception& e) {
            expect (false, e.what());
        }
    }

private:
    std::unique_ptr<float []> values;
    float* rawValues = nullptr;
    sol::state lua;
};

static CArrayTest sCArrayTest;


class LuaTableTest : public UnitTestBase
{
public:
    LuaTableTest() : UnitTestBase ("Lua Table", "Lua", "table") {}
    virtual ~LuaTableTest() { }
    void initialise() override
    {
        lua.open_libraries();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        // beginTest ("file");

        // auto result = lua.safe_script (R"(
        //     local function node_save()
        //         io.write ("hello world")
        //     end

        //     local tf = io.tmpfile()
        //     local oi = io.output()
        //     io.output (tf);
        //     node_save()
        //     tf:seek ("set", 0)
        //     local s = tf:read ("*a")
        //     io.close()
        //     io.output (of);

        //     return s
        // )");

        // expect (result.valid());
        // if (result.valid())
        // {
        //     sol::object ret = result;
        //     if (ret.get_type() == sol::type::string)
        //     {
        //         DBG("got a result");
        //         DBG(ret.as<const char*>() << " len: " << strlen(ret.as<const char*>()));
        //     }
        //     else
        //     {
        //         expect(false, "didn't get a string");
        //     }
        // }

        beginTest("memory file");
        const char* testData = "This is some test data";
        // void* buffer = malloc (testData.length());
        // memcpy (buffer, testData.toRawUTF8(), testData.length());

        sol::userdata ud = lua.script ("return io.tmpfile()");
        FILE* file = ud.as<FILE*>();
        fwrite (testData, 1, strlen (testData), file);

        lua["__state_data__"] = ud;
        lua.script (R"(
            local oi = io.input()
            __state_data__:seek ("set", 0)
            io.input (__state_data__)
            --node_restore()
            print (io.read("*a"))
            io.input(oi)
            __state_data__:close()
            __state_data__ = nil
        )");

        lua.collect_garbage();

        beginTest ("table");

        auto obj = lua.script (R"(
        function table_value_tree (e)
            -- if e is a table, we should iterate over its elements
            if type(e) == "table" then
                for k,v in pairs(e) do -- for every element in the table
                    print(k)
                    table_value_tree (v)       -- recursively repeat the same procedure
                end
            else -- if not, we can just print it
                print(e)
            end
        end

        return {
            100,
            key0 = 100,
            key1 = 200,
            key2 = 300,
            key3 = {
                key1 = 100,
                key2 = 200,
                key3 = 300
            }
        } 
        )");

        lua["table_value_tree"](obj);

        sol::table t = obj;
        return;
        DBG("size = " << t.size());
        for (int i = 1; i <= t.size(); ++i)
        {
            sol::object o = t [i];
            DBG("index = " << i);

            switch (o.get_type())
            {
                case sol::type::table:
                    DBG("table");
                    break;
                case sol::type::boolean:
                    DBG("boolean");
                    break;
                case sol::type::string:
                    DBG("string");
                    break;
                case sol::type::number:
                    DBG("number");
                    break;

                case sol::type::thread:
                    DBG("thread");
                    break;
                case sol::type::lua_nil:
                    DBG("nil");
                    break;
                case sol::type::none:
                    DBG("none");
                    break;
                case sol::type::function:
                    DBG("function");
                    break;
                case sol::type::userdata:
                    DBG("userdata");
                    break;
                case sol::type::lightuserdata:
                    DBG("lightuserdata");
                    break;
                case sol::type::poly:
                    DBG("poly");
                    break;
            }
        }
    }

private:
    sol::state lua;
};

static LuaTableTest sLuaTableTest;


class LuaGlobalsTest : public UnitTestBase
{
public:
    LuaGlobalsTest() : UnitTestBase ("Lua Globals", "Lua", "globals") {}
    virtual ~LuaGlobalsTest() { }
    void initialise() override
    {
        lua.open_libraries();
        Lua::setWorld (lua, &getWorld());
        Lua::registerModel (lua);
        Lua::registerElement (lua);
        lua["expect"] = [this](bool result, const char* msg) {
            this->expect (result, msg);
        };
    }

    void shutdown() override
    {
        Lua::setWorld (lua, nullptr);
        lua.collect_garbage();
        shutdownWorld();
    }

    void runTest() override
    {
        testSession();
    }

private:
    sol::state lua;

    void testSession()
    {
        beginTest("session");
        auto s = getWorld().getSession();
        s->setName ("test session");
        
        try
        {
            auto result = lua.safe_script (R"(
                local s = Element:session()
                expect (s:get_name() == "test session", "incorrect session name")
                s:set_name ("lua session")
                expect (s:get_name() == "lua session", "wrong session name")
                s:add_graph (Node:create_graph(), false)
                s:add_graph (Node:create_graph(), false)
                expect (s:get_num_graphs() == 2, "incorrect number of graphs")
            )");

            expect (result.valid());
            expect (s->getNumGraphs() == 2);
            expect (s->getName() == "lua session");
        }
        catch (const std::exception& e)
        {
            expect (false, e.what());
        }
    }
};

static LuaGlobalsTest sLuaGlobalsTest;

//=============================================================================

namespace Element {
extern int audiobuffer_new (lua_State* L);
extern int luaopen_audiobuffer (lua_State* L);
}

class LuaAudioBufferTest : public UnitTestBase
{
public:
    LuaAudioBufferTest() : UnitTestBase ("Lua Globals", "Lua", "audiobuffer") {}
    virtual ~LuaAudioBufferTest() { }
    
    void initialise() override
    {
        lua.open_libraries();
        auto* L = lua.lua_state();
        jassert (lua_gettop (L) == 0);
        lua_settop (L, 0);
       #if 1
        lua_newtable (L);
        lua_setglobal (L, "audio");
        lua_getglobal (L, "audio");
        jassert(lua_gettop(L) == 1);
        // DBG("stack = " << lua_gettop (L));
        luaopen_audiobuffer (L);
        // DBG("stack = " << lua_gettop (L));
        lua_pushcfunction (L, audiobuffer_new);
        lua_setfield (L, 1, "Buffer");
        // DBG("stack = " << lua_gettop (L));
        for (int i = lua_gettop(L); --i > 0;)
            lua_pop (L, i);
        
       #else
        luaL_requiref (L, "AudioBuffer", luaopen_audiobuffer, 1);
        lua_pop (L, 1);
        DBG("stack = " << lua_gettop (L));
       #endif
        lua.collect_garbage();
    }

    void shutdown() override
    {
        lua.collect_garbage();
    }

    void runTest() override
    {
        beginTest ("buffer");
        lua.script (R"(
            print ("audio.Buffer", audio.Buffer)
            local b = audio.Buffer()
            print(string.format ("type: %s", tostring (b)))
            print("nframes: ", b:length())
        )");
    }

private:
    sol::state lua;
};

static LuaAudioBufferTest sLuaAudioBufferTest;

#endif
