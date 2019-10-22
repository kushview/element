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

using namespace Element;

namespace Element {

class PortListTest : public UnitTestBase
{
public:
    explicit PortListTest()
        : UnitTestBase ("PortList", "nodes", "portList") { }
    virtual ~PortListTest() { }

    void runTest() override
    {
        testControlAndMidiOnly();
    }

private:
    void testControlAndMidiOnly()
    {
        beginTest ("control & midi ports only");
        kv::PortList ports;
        int index = 0;
        for (int control = 0; control < 16; ++control)
        {
            String symbol = "control_"; symbol << control;
            ports.add (PortType::Control, index++, control,
                       symbol, "Control", true);
        }

        ports.add (PortType::Midi, index++, 0, "midi_in", "MIDI", true);
        ports.add (PortType::Midi, index++, 0, "midi_out", "MIDI", false);

        for (int port = 0; port < 16; ++port)
        {
            expect (ports.getType (port) == PortType::Control);
            expect (ports.isInput (port));
            expect (ports.getChannelForPort (port) == port);
        }

        const PortType midiType (PortType::Midi);
        expect (ports.isInput(16) == true);
        expect (ports.getType(16) == midiType);
        expect (ports.isOutput(17) == true);
        expect (ports.getType(17) == midiType);
    }
};

static PortListTest sPortListTest;

}
