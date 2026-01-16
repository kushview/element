// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/processor.hpp>

#include "nodes/midiprogrammap.hpp"
#include "engine/graphnode.hpp"

using namespace element;
using namespace juce;

static void testMappings (ProcessorPtr node)
{
    BOOST_REQUIRE_EQUAL (node->getNumPorts (PortType::Midi, true), 1);
    BOOST_REQUIRE_EQUAL (node->getNumPorts (PortType::Midi, false), 1);
    auto* pgc = dynamic_cast<MidiProgramMapNode*> (node.get());
    pgc->clear();
    MidiProgramMapNode::ProgramEntry entry;

    pgc->addProgramEntry ("Program 1", 3, 4);
    entry = pgc->getProgramEntry (0);
    BOOST_REQUIRE (entry.name == "Program 1" && entry.in == 3 && entry.out == 4);

    pgc->addProgramEntry ("Program 2", 5);
    entry = pgc->getProgramEntry (1);
    BOOST_REQUIRE (entry.in == 5 && entry.out == 5);

    pgc->addProgramEntry ("Program Edit 1", 3, 6);
    entry = pgc->getProgramEntry (0);
    BOOST_REQUIRE (entry.in = 3 && entry.out == 6);
    BOOST_REQUIRE_EQUAL (pgc->getNumProgramEntries(), 2);

    pgc->editProgramEntry (0, "Program Edit 2", 10, 6);
    BOOST_REQUIRE (entry.in = 10 && entry.out == 6);
    BOOST_REQUIRE_EQUAL (pgc->getNumProgramEntries(), 2);
}

static void testMidiStream (ProcessorPtr node, const String& name = "Renders mappings")
{
    MidiBuffer midi;
    AudioSampleBuffer audio, cv;
    audio.setSize (2, 1024, false, true, false);

    midi.addEvent (MidiMessage::programChange (1, 10), 100);
    midi.addEvent (MidiMessage::programChange (1, 5), 200);
    midi.addEvent (MidiMessage::noteOn (1, 12, static_cast<uint8> (50)), 300);
    midi.addEvent (MidiMessage::noteOff (1, 12), 300);

    RenderContext rc (audio, cv, midi, audio.getNumSamples());
    node->render (rc);

    int index = 0;
    for (auto m : midi) {
        auto msg = m.getMessage();
        switch (index) {
            case 0:
                BOOST_REQUIRE (msg.isProgramChange() && msg.getProgramChangeNumber() == 6);
                break;
            case 1:
                BOOST_REQUIRE (msg.isProgramChange() && msg.getProgramChangeNumber() == 5);
                break;
            case 2:
                BOOST_REQUIRE (msg.isNoteOn());
                break;
            case 3:
                BOOST_REQUIRE (msg.isNoteOff());
                break;
        }

        ++index;
    }
}

BOOST_AUTO_TEST_SUITE (MidiProgramMapTests)

BOOST_AUTO_TEST_CASE (Basics)
{
    Context context;
    {
        GraphNode graph (context);
        ProcessorPtr node = graph.addNode (new MidiProgramMapNode());
        auto* pgc = dynamic_cast<MidiProgramMapNode*> (node.get());

        testMappings (node);
        testMidiStream (node);

        MemoryBlock block;
        pgc->getState (block);
        pgc->clear();

        BOOST_REQUIRE_EQUAL (pgc->getNumProgramEntries(), 0);
        pgc->setState (block.getData(), (int) block.getSize());

        testMidiStream (node, "Renders mappings after load state");

        pgc = nullptr;
        node = nullptr;
        graph.clear();
    }
}

BOOST_AUTO_TEST_SUITE_END()
