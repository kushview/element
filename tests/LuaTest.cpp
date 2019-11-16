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

#include "scripting/LuaBindings.h"
#include "sol/sol.hpp"

using namespace Element;

static const std::string nodeScript = R"(
if element and element.plugin then
    element.plugin ({
        type        = "node",
        name        = "Test Node",
        author      = "Michael Fisher",
        description = [[ Test script registration ]]
    })
end

function node_ports()
    return {
        audio_ins   = 2,
        audio_outs  = 2,
        midi_ins    = 1
    }
end

function node_prepare (sample_rate, block_size)

end

function node_render (audio, midi)

end

function node_release()

end
)";

class LuaTest : public UnitTestBase
{
public:
    LuaTest() : UnitTestBase ("LuaNode", "Lua", "node") {}
    virtual ~LuaTest() { }
    void initialise() override
    {
        lua.open_libraries();
        Element::Lua::registerEngine (lua);
    }

    void runTest() override
    {
        beginTest ("ports");
        lua.script (nodeScript);

        kv::PortList ports;
        createPorts (ports);

        expect (ports.size (PortType::Audio, true) == 2);
        expect (ports.size (PortType::Audio, false) == 2);
        expect (ports.size (PortType::Midi,  true) == 1);
        expect (ports.size (PortType::Midi,  false) == 0);
        expect (ports.size() == 5);

        for (const auto* port : ports.getPorts()) {
            DBG(port->name << " : " << port->symbol << " : " << port->channel);
        }
    }

    void createPorts (kv::PortList& ports)
    {
        if (auto f = lua ["node_ports"])
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

private:
    sol::state lua;
};

static LuaTest sLuaTest;
#endif
