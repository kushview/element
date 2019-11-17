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
    print("prepare test node")
    prepared_flag = true
end

function node_render (audio, midi)

end

function node_release()
    prepared_flag = false
end
)";

class LuaNodeTest : public UnitTestBase
{
public:
    LuaNodeTest() : UnitTestBase ("LuaNode", "Lua", "node") {}
    virtual ~LuaNodeTest() { }
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

        beginTest ("prepare");
        node->loadScript (nodeScript);
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

static LuaNodeTest sLuaNodeTest;
#endif
