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
#include "engine/nodes/MidiProgramMapNode.h"
#include "engine/graphnode.hpp"

namespace element {

class ProgramChangeMapTest : public UnitTestBase {
public:
    ProgramChangeMapTest() : UnitTestBase ("Program Change Map", "nodes", "midiProgramMap") {}
    void initialise() override {}
    void shutdown() override {}
    void runTest() override
    {
        GraphNode graph;
        NodeObjectPtr node = graph.addNode (new MidiProgramMapNode());
        auto* pgc = dynamic_cast<MidiProgramMapNode*> (node.get());

        testMappings (node);
        testMidiStream (node);

        MemoryBlock block;

        pgc->getState (block);
        beginTest ("::clear()");
        pgc->clear();
        expect (pgc->getNumProgramEntries() == 0);
        pgc->setState (block.getData(), (int) block.getSize());

        testMidiStream (node, "Renders mappings after load state");

        pgc = nullptr;
        node = nullptr;
        graph.clear();
    }

private:
    void testMappings (NodeObjectPtr node)
    {
        beginTest ("port count");
        expect (node->getNumPorts (PortType::Midi, true) == 1);
        expect (node->getNumPorts (PortType::Midi, false) == 1);
        auto* pgc = dynamic_cast<MidiProgramMapNode*> (node.get());
        pgc->clear();
        MidiProgramMapNode::ProgramEntry entry;

        beginTest ("3 maps to 4");
        pgc->addProgramEntry ("Program 1", 3, 4);
        entry = pgc->getProgramEntry (0);
        expect (entry.name == "Program 1" && entry.in == 3 && entry.out == 4);

        beginTest ("1 to 1 map");
        pgc->addProgramEntry ("Program 2", 5);
        entry = pgc->getProgramEntry (1);
        expect (entry.in == 5 && entry.out == 5);

        beginTest ("does not duplicate");
        pgc->addProgramEntry ("Program Edit 1", 3, 6);
        entry = pgc->getProgramEntry (0);
        expect (entry.in = 3 && entry.out == 6);
        expect (pgc->getNumProgramEntries() == 2);

        beginTest ("edit program");
        pgc->editProgramEntry (0, "Program Edit 2", 10, 6);
        expect (entry.in = 10 && entry.out == 6);
        expect (pgc->getNumProgramEntries() == 2);
    }

    void testMidiStream (NodeObjectPtr node, const String& name = "Renders mappings")
    {
        beginTest (name);
        OwnedArray<MidiBuffer> buffers;
        Array<int> channels;
        buffers.add (new MidiBuffer());
        channels.add (0);

        MidiPipe pipe (buffers, channels);
        AudioSampleBuffer audio;
        audio.setSize (2, 1024, false, true, false);

        auto* midi = pipe.getWriteBuffer (0);
        midi->addEvent (MidiMessage::programChange (1, 10), 100);
        midi->addEvent (MidiMessage::programChange (1, 5), 200);
        midi->addEvent (MidiMessage::noteOn (1, 12, static_cast<uint8> (50)), 300);
        midi->addEvent (MidiMessage::noteOff (1, 12), 300);
        node->render (audio, pipe);

        MidiBuffer::Iterator iter (*midi);
        MidiMessage msg;
        int frame = 0, index = 0;
        while (iter.getNextEvent (msg, frame)) {
            switch (index) {
                case 0:
                    expect (msg.isProgramChange() && msg.getProgramChangeNumber() == 6);
                    break;
                case 1:
                    expect (msg.isProgramChange() && msg.getProgramChangeNumber() == 5);
                    break;
                case 2:
                    expect (msg.isNoteOn());
                    break;
                case 3:
                    expect (msg.isNoteOff());
                    break;
            }

            ++index;
        }
    }
};

static ProgramChangeMapTest sProgramChangeMapTest;

} // namespace element
