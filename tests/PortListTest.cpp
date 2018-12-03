/*
    DummyTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
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
