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
#include "engine/nodes/AudioProcessorNode.h"
#include "engine/ionode.hpp"
#include "engine/MidiChannelMap.h"
#include "engine/nodes/MidiChannelMapProcessor.h"

using namespace element;

namespace element {

class MidiChannelMapTest : public UnitTestBase
{
public:
    explicit MidiChannelMapTest (const String& name = "MidiChannelMap") 
        : UnitTestBase (name, "engine", "midiChannelMap") { }
    virtual ~MidiChannelMapTest() { }

    void initialise() override {
        MessageManager::getInstance();
    }

    void runTest() override
    {
        testPortTypes();
        testPortConfig();
        testReset();
        testOutputCorrect();
        testProcess();
    }

private:
    void testProcess()
    {
        beginTest ("process()");
        MidiChannelMap chmap;
        MidiMessage note (MidiMessage::noteOn (5, 100, 1.f));
        chmap.set (5, 6);
        chmap.process (note);
        expect (chmap.get (5) == 6);
    }

    void testReset()
    {
        beginTest ("reset()");
        MidiChannelMap chmap;
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 17 - ch);
        chmap.reset();
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get(ch) == ch);
    }

    void testOutputCorrect()
    {
        beginTest ("get() and set()");
        MidiChannelMap chmap;
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get(ch) == ch);
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 17 - ch);
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get (ch) == 17 - ch);
        for (int ch = 1; ch <= 16; ++ch)
            chmap.set (ch, 12);
        for (int ch = 1; ch <= 16; ++ch)
            expect (chmap.get (ch) == 12);
    }

    void testPortTypes()
    {
        beginTest ("port types");
        const PortType midi (String ("midi"));
        expect (midi.isMidi());
        const PortType control (String ("control"));
        expect (control.isControl());
    }

    void testPortConfig()
    {
        beginTest ("port config");
        GraphNode proc;
        proc.setRenderDetails (44100.f, 512);
        proc.prepareToRender (44100.0, 512);
        auto* processor = new MidiChannelMapProcessor();
        expect (processor->getTotalNumInputChannels() == 0);
        expect (processor->getTotalNumOutputChannels() == 0);
        
        NodeObjectPtr node = proc.addNode (new AudioProcessorNode (processor));
        MessageManager::getInstance()->runDispatchLoopUntil (10);
        expect (processor->getTotalNumInputChannels() == 0);
        expect (processor->getTotalNumOutputChannels() == 0);
        
        expect (node != nullptr);

        expect (node->getNumAudioInputs() == 0);
        expect (node->getNumAudioOutputs() == 0);
        expect (node->getNumPorts (PortType::Audio, true) == 0);
        expect (node->getNumPorts (PortType::Audio, false) == 0);
        expect (node->getNumPorts (PortType::Control, true) == 16);
        expect (node->getNumPorts (PortType::Control, false) == 0);
        expect (node->getNumPorts (PortType::Midi, true) == 1);
        expect (node->getNumPorts (PortType::Midi, false) == 1);
        expect (node->getNumPorts() == 18);

        for (int i = 0; i < 16; ++i)
        {
            expect (node->getPortType(i) == PortType::Control);
            expect (node->isPortInput (i));
        }

        expect (node->getPortType (16) == PortType::Midi);
        expect (node->isPortInput (16));
        expect (node->getMidiInputPort() == 16);
        expect (node->getPortType (17) == PortType::Midi);
        expect (! node->isPortInput (17));
        expect (node->getMidiOutputPort() == 17);

        expect (node->getPortType (18) == PortType::Unknown);

        NodeObjectPtr midiIn = proc.addNode (new element::IONode (element::IONode::midiInputNode));

        beginTest ("connectivity");
        expect (midiIn->getNumPorts() == 1);
        expect (midiIn->getPortType (0) == PortType::Midi);
        expect (midiIn->isPortInput (0) == false);
        expect (proc.addConnection (midiIn->nodeId, midiIn->getMidiOutputPort(),
                                    node->nodeId, node->getMidiInputPort()));

        node = nullptr;
        midiIn = nullptr;
        proc.releaseResources();
        proc.clear();
    }
};

static MidiChannelMapTest sMidiChannelMapTest;

}
