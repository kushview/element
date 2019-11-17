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

function node_render (audio, midi)
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

function node_render (audio, midi)
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

function node_render (audio, midi)
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

function node_render (audio, midi)
    local mb = midi:get_write_buffer (2)
    mb:clear()
end

function node_release()
end
)";

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
        node->loadScript (nodeScript);
        
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

class LuaNodeValidateTest : public UnitTestBase
{
public:
    LuaNodeValidateTest() : UnitTestBase ("Lua Node Validation", "LuaNode", "validate") { }
    virtual ~LuaNodeValidateTest() { }
    void initialise() override { }

    void runTest() override
    {
        auto node = std::make_unique<LuaNode>();
        beginTest ("global syntax error");
        auto result = node->loadScript (globalSyntaxError);
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

#endif
