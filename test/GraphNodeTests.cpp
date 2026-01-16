// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>

#include <lv2/atom/atom.h>
#include <lv2/midi/midi.h>

#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "fixture/MidiGeneratorNode.h"
#include "fixture/MidiCaptureNode.h"
#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"

using namespace element;

BOOST_AUTO_TEST_SUITE (GraphNodeTests)

BOOST_AUTO_TEST_CASE (IO)
{
    GraphNode graph (*element::test::context());
    BOOST_REQUIRE_EQUAL (graph.getNumAudioInputs(), 2);
    BOOST_REQUIRE_EQUAL (graph.getNumAudioOutputs(), 2);
    graph.clear();
}

BOOST_AUTO_TEST_CASE (Connections)
{
    GraphNode graph (*element::test::context());
    {
        auto* node1 = new TestNode();
        graph.addNode (node1);
        BOOST_REQUIRE ((void*) node1 == (void*) graph.getNodeForId (node1->nodeId));

        auto* node2 = new TestNode();
        graph.addNode (node2);
        BOOST_REQUIRE ((void*) node2 == (void*) graph.getNodeForId (node2->nodeId));

        BOOST_REQUIRE_EQUAL (graph.getNumNodes(), 2);
        BOOST_REQUIRE_EQUAL (graph.getNumConnections(), 0);
        const auto srcAudioPort = node1->getPortForChannel (PortType::Audio, 0, false);
        const auto srcMidiPort = node1->getPortForChannel (PortType::Midi, 0, false);
        const auto dstAudioPort = node2->getPortForChannel (PortType::Audio, 0, true);
        const auto dstMidiPort = node2->getPortForChannel (PortType::Midi, 0, true);

        BOOST_REQUIRE (graph.canConnect (node1->nodeId, srcAudioPort, node2->nodeId, dstAudioPort));
        BOOST_REQUIRE (graph.canConnect (node1->nodeId, srcMidiPort, node2->nodeId, dstMidiPort));
        BOOST_REQUIRE (! graph.canConnect (node1->nodeId, srcMidiPort, node2->nodeId, dstAudioPort));

        BOOST_REQUIRE (! graph.canConnect (9999, 0, node2->nodeId, 0));
        BOOST_REQUIRE (! graph.canConnect (node1->nodeId, 3, 9998, 0));

        BOOST_REQUIRE (! graph.canConnect (node1->nodeId, 100, node2->nodeId, 3));
        BOOST_REQUIRE (! graph.canConnect (node1->nodeId, 5, node2->nodeId, 100));

        BOOST_REQUIRE (graph.addConnection (node1->nodeId, srcAudioPort, node2->nodeId, dstAudioPort));
        BOOST_REQUIRE (! graph.canConnect (node1->nodeId, srcAudioPort, node2->nodeId, dstAudioPort));
        BOOST_REQUIRE (! graph.canConnect (node1->nodeId, srcAudioPort, node1->nodeId, dstAudioPort));

        BOOST_REQUIRE (graph.connectChannels (PortType::Audio, node1->nodeId, 1, node2->nodeId, 1));
        BOOST_REQUIRE (graph.isConnected (node1->nodeId, node2->nodeId));
        BOOST_REQUIRE (nullptr != graph.getConnectionBetween (node1->nodeId, srcAudioPort, node2->nodeId, dstAudioPort));
        BOOST_REQUIRE (nullptr == graph.getConnectionBetween (node1->nodeId, srcAudioPort, node2->nodeId, 111111));
        BOOST_REQUIRE_EQUAL (graph.getNumConnections(), 2);

        graph.removeNode (node1->nodeId);
        graph.removeNode (node2->nodeId);
        BOOST_REQUIRE_EQUAL (graph.getNumConnections(), 0);
        BOOST_REQUIRE_EQUAL (graph.getNumNodes(), 0);
    }
    graph.clear();
}

BOOST_AUTO_TEST_CASE (AddRemove)
{
    PreparedGraph fix;
    GraphNode& graph = fix.graph;
    ProcessorPtr node = graph.addNode (new TestNode());
    graph.rebuild();
    BOOST_REQUIRE (graph.getNumNodes() == 1);
    BOOST_REQUIRE (node != nullptr);
    BOOST_REQUIRE (graph.removeNode (node->nodeId));
}

BOOST_AUTO_TEST_CASE (MultiMidiToSingleMidi)
{
    // Test multiple MIDI sources -> single MIDI destination
    // Verifies that MIDI merging works correctly

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiSrc1 = new MidiGeneratorNode();
    auto* midiSrc2 = new MidiGeneratorNode();
    auto* midiDst = new MidiCaptureNode();

    graph.addNode (midiSrc1);
    graph.addNode (midiSrc2);
    graph.addNode (midiDst);

    // Connect both MIDI outputs to single MIDI input
    const auto midi1OutPort = midiSrc1->getPortForChannel (PortType::Midi, 0, false);
    const auto midi2OutPort = midiSrc2->getPortForChannel (PortType::Midi, 0, false);
    const auto midiInPort = midiDst->getPortForChannel (PortType::Midi, 0, true);

    BOOST_REQUIRE (graph.addConnection (midiSrc1->nodeId, midi1OutPort, midiDst->nodeId, midiInPort));
    BOOST_REQUIRE (graph.addConnection (midiSrc2->nodeId, midi2OutPort, midiDst->nodeId, midiInPort));

    graph.rebuild();

    // Generate MIDI events - 3 from each source = 6 total
    midiSrc1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midiSrc1->addEvent (MidiMessage::noteOn (1, 64, 0.8f), 10);
    midiSrc1->addEvent (MidiMessage::noteOff (1, 60), 20);

    midiSrc2->addEvent (MidiMessage::noteOn (1, 67, 0.7f), 5);
    midiSrc2->addEvent (MidiMessage::noteOn (1, 71, 0.7f), 15);
    midiSrc2->addEvent (MidiMessage::noteOff (1, 67), 25);

    // Render the graph
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify all 6 events arrived at the MIDI destination
    const int actualEvents = midiDst->getEventCount();
    BOOST_REQUIRE_EQUAL (actualEvents, 6);
}

BOOST_AUTO_TEST_CASE (SingleMidiToMultiMidi)
{
    // Test single MIDI source -> multiple MIDI destinations
    // Verifies that MIDI is properly copied to all destinations

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiSrc = new MidiGeneratorNode();
    auto* midiDst1 = new MidiCaptureNode();
    auto* midiDst2 = new MidiCaptureNode();

    graph.addNode (midiSrc);
    graph.addNode (midiDst1);
    graph.addNode (midiDst2);

    const auto midiOutPort = midiSrc->getPortForChannel (PortType::Midi, 0, false);
    const auto midiIn1Port = midiDst1->getPortForChannel (PortType::Midi, 0, true);
    const auto midiIn2Port = midiDst2->getPortForChannel (PortType::Midi, 0, true);

    BOOST_REQUIRE (graph.addConnection (midiSrc->nodeId, midiOutPort, midiDst1->nodeId, midiIn1Port));
    BOOST_REQUIRE (graph.addConnection (midiSrc->nodeId, midiOutPort, midiDst2->nodeId, midiIn2Port));

    graph.rebuild();

    midiSrc->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midiSrc->addEvent (MidiMessage::noteOn (1, 64, 0.8f), 10);
    midiSrc->addEvent (MidiMessage::noteOff (1, 60), 20);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Both destinations should receive all 3 events
    BOOST_REQUIRE_EQUAL (midiDst1->getEventCount(), 3);
    BOOST_REQUIRE_EQUAL (midiDst2->getEventCount(), 3);
}

BOOST_AUTO_TEST_CASE (MidiIsolation)
{
    // Test that MIDI doesn't leak between unconnected nodes
    // Source1 -> Dest1, Source2 -> Dest2 (no cross connections)

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiSrc1 = new MidiGeneratorNode();
    auto* midiSrc2 = new MidiGeneratorNode();
    auto* midiDst1 = new MidiCaptureNode();
    auto* midiDst2 = new MidiCaptureNode();

    graph.addNode (midiSrc1);
    graph.addNode (midiSrc2);
    graph.addNode (midiDst1);
    graph.addNode (midiDst2);

    const auto midi1OutPort = midiSrc1->getPortForChannel (PortType::Midi, 0, false);
    const auto midi2OutPort = midiSrc2->getPortForChannel (PortType::Midi, 0, false);
    const auto midiIn1Port = midiDst1->getPortForChannel (PortType::Midi, 0, true);
    const auto midiIn2Port = midiDst2->getPortForChannel (PortType::Midi, 0, true);

    // Connect src1->dst1 and src2->dst2 (isolated paths)
    BOOST_REQUIRE (graph.addConnection (midiSrc1->nodeId, midi1OutPort, midiDst1->nodeId, midiIn1Port));
    BOOST_REQUIRE (graph.addConnection (midiSrc2->nodeId, midi2OutPort, midiDst2->nodeId, midiIn2Port));

    graph.rebuild();

    // Src1: 2 events, Src2: 3 events
    midiSrc1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midiSrc1->addEvent (MidiMessage::noteOff (1, 60), 10);

    midiSrc2->addEvent (MidiMessage::noteOn (1, 67, 0.7f), 5);
    midiSrc2->addEvent (MidiMessage::noteOn (1, 71, 0.7f), 15);
    midiSrc2->addEvent (MidiMessage::noteOff (1, 67), 25);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Each destination should only receive events from its connected source
    BOOST_REQUIRE_EQUAL (midiDst1->getEventCount(), 2);
    BOOST_REQUIRE_EQUAL (midiDst2->getEventCount(), 3);
}

BOOST_AUTO_TEST_CASE (DisconnectedMidiNode)
{
    // Test that a disconnected node receives no MIDI

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiSrc = new MidiGeneratorNode();
    auto* midiDst = new MidiCaptureNode();

    graph.addNode (midiSrc);
    graph.addNode (midiDst);

    // Don't connect them
    graph.rebuild();

    midiSrc->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midiSrc->addEvent (MidiMessage::noteOn (1, 64, 0.8f), 10);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Destination should receive no events
    BOOST_REQUIRE_EQUAL (midiDst->getEventCount(), 0);
}

BOOST_AUTO_TEST_CASE (MidiThroughIONodes)
{
    // Test MIDI routing through graph I/O: External MIDI In -> Internal Node -> External MIDI Out

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    // Create MIDI I/O nodes explicitly
    auto* midiInputNode = new IONode (IONode::midiInputNode);
    auto* midiOutputNode = new IONode (IONode::midiOutputNode);

    graph.addNode (midiInputNode);
    graph.addNode (midiOutputNode);

    // Add a processor in the middle
    auto* processor = new MidiCaptureNode();
    graph.addNode (processor);

    // Connect: MIDI In -> Processor -> MIDI Out
    const auto midiInOutPort = midiInputNode->getPortForChannel (PortType::Midi, 0, false);
    const auto processorInPort = processor->getPortForChannel (PortType::Midi, 0, true);
    const auto midiOutInPort = midiOutputNode->getPortForChannel (PortType::Midi, 0, true);

    BOOST_REQUIRE (graph.addConnection (midiInputNode->nodeId, midiInOutPort, processor->nodeId, processorInPort));

    graph.rebuild();

    // Send MIDI into the graph via the RenderContext
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.8f), 10);
    midi.addEvent (MidiMessage::noteOff (1, 60), 20);

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Processor should receive the 3 MIDI events from graph input
    BOOST_REQUIRE_EQUAL (processor->getEventCount(), 3);
}

BOOST_AUTO_TEST_CASE (ComplexMidiRouting)
{
    // Complex routing: 3 sources (2 generators + 1 I/O) -> 4 destinations
    // Tests many-to-many with mixed connection patterns

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiIn = new IONode (IONode::midiInputNode);
    auto* gen1 = new MidiGeneratorNode();
    auto* gen2 = new MidiGeneratorNode();
    auto* dst1 = new MidiCaptureNode(); // Gets from gen1 only
    auto* dst2 = new MidiCaptureNode(); // Gets from gen2 only
    auto* dst3 = new MidiCaptureNode(); // Gets from midiIn + gen1
    auto* dst4 = new MidiCaptureNode(); // Gets from all 3 sources

    graph.addNode (midiIn);
    graph.addNode (gen1);
    graph.addNode (gen2);
    graph.addNode (dst1);
    graph.addNode (dst2);
    graph.addNode (dst3);
    graph.addNode (dst4);

    const auto midiInOut = midiIn->getPortForChannel (PortType::Midi, 0, false);
    const auto gen1Out = gen1->getPortForChannel (PortType::Midi, 0, false);
    const auto gen2Out = gen2->getPortForChannel (PortType::Midi, 0, false);
    const auto dst1In = dst1->getPortForChannel (PortType::Midi, 0, true);
    const auto dst2In = dst2->getPortForChannel (PortType::Midi, 0, true);
    const auto dst3In = dst3->getPortForChannel (PortType::Midi, 0, true);
    const auto dst4In = dst4->getPortForChannel (PortType::Midi, 0, true);

    // dst1 <- gen1
    BOOST_REQUIRE (graph.addConnection (gen1->nodeId, gen1Out, dst1->nodeId, dst1In));

    // dst2 <- gen2
    BOOST_REQUIRE (graph.addConnection (gen2->nodeId, gen2Out, dst2->nodeId, dst2In));

    // dst3 <- midiIn + gen1
    BOOST_REQUIRE (graph.addConnection (midiIn->nodeId, midiInOut, dst3->nodeId, dst3In));
    BOOST_REQUIRE (graph.addConnection (gen1->nodeId, gen1Out, dst3->nodeId, dst3In));

    // dst4 <- midiIn + gen1 + gen2
    BOOST_REQUIRE (graph.addConnection (midiIn->nodeId, midiInOut, dst4->nodeId, dst4In));
    BOOST_REQUIRE (graph.addConnection (gen1->nodeId, gen1Out, dst4->nodeId, dst4In));
    BOOST_REQUIRE (graph.addConnection (gen2->nodeId, gen2Out, dst4->nodeId, dst4In));

    graph.rebuild();

    // midiIn: 2 events
    // gen1: 3 events
    // gen2: 4 events
    gen1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    gen1->addEvent (MidiMessage::noteOn (1, 64, 0.8f), 10);
    gen1->addEvent (MidiMessage::noteOff (1, 60), 20);

    gen2->addEvent (MidiMessage::noteOn (1, 67, 0.7f), 5);
    gen2->addEvent (MidiMessage::noteOn (1, 71, 0.7f), 15);
    gen2->addEvent (MidiMessage::noteOn (1, 74, 0.7f), 25);
    gen2->addEvent (MidiMessage::noteOff (1, 67), 35);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 48, 0.9f), 8);
    midi.addEvent (MidiMessage::noteOff (1, 48), 18);

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify event counts
    BOOST_REQUIRE_EQUAL (dst1->getEventCount(), 3); // gen1 only
    BOOST_REQUIRE_EQUAL (dst2->getEventCount(), 4); // gen2 only
    BOOST_REQUIRE_EQUAL (dst3->getEventCount(), 5); // midiIn(2) + gen1(3)
    BOOST_REQUIRE_EQUAL (dst4->getEventCount(), 9); // midiIn(2) + gen1(3) + gen2(4)
}

BOOST_AUTO_TEST_CASE (MidiChainWithBranching)
{
    // Test complex chain with branching:
    // Src1 -> Pass1 -> Dst1
    //              \-> Dst2
    // Src2 -> Pass2 -> Dst2
    //              \-> Dst3

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    class MidiPassNode : public TestNode {
    public:
        MidiPassNode() : TestNode (0, 0, 1, 1) {}

        void render (RenderContext& rc) override
        {
            if (rc.midi.getNumBuffers() >= 2) {
                auto* input = rc.midi.getWriteBuffer (0);
                auto* output = rc.midi.getWriteBuffer (1);
                output->clear();
                output->addEvents (*input, 0, 512, 0);
            }
        }
    };

    auto* src1 = new MidiGeneratorNode();
    auto* src2 = new MidiGeneratorNode();
    auto* pass1 = new MidiPassNode();
    auto* pass2 = new MidiPassNode();
    auto* dst1 = new MidiCaptureNode();
    auto* dst2 = new MidiCaptureNode();
    auto* dst3 = new MidiCaptureNode();

    graph.addNode (src1);
    graph.addNode (src2);
    graph.addNode (pass1);
    graph.addNode (pass2);
    graph.addNode (dst1);
    graph.addNode (dst2);
    graph.addNode (dst3);

    const auto src1Out = src1->getPortForChannel (PortType::Midi, 0, false);
    const auto src2Out = src2->getPortForChannel (PortType::Midi, 0, false);
    const auto pass1In = pass1->getPortForChannel (PortType::Midi, 0, true);
    const auto pass1Out = pass1->getPortForChannel (PortType::Midi, 0, false);
    const auto pass2In = pass2->getPortForChannel (PortType::Midi, 0, true);
    const auto pass2Out = pass2->getPortForChannel (PortType::Midi, 0, false);
    const auto dst1In = dst1->getPortForChannel (PortType::Midi, 0, true);
    const auto dst2In = dst2->getPortForChannel (PortType::Midi, 0, true);
    const auto dst3In = dst3->getPortForChannel (PortType::Midi, 0, true);

    // Src1 -> Pass1
    BOOST_REQUIRE (graph.addConnection (src1->nodeId, src1Out, pass1->nodeId, pass1In));
    // Pass1 -> Dst1 and Dst2
    BOOST_REQUIRE (graph.addConnection (pass1->nodeId, pass1Out, dst1->nodeId, dst1In));
    BOOST_REQUIRE (graph.addConnection (pass1->nodeId, pass1Out, dst2->nodeId, dst2In));

    // Src2 -> Pass2
    BOOST_REQUIRE (graph.addConnection (src2->nodeId, src2Out, pass2->nodeId, pass2In));
    // Pass2 -> Dst2 and Dst3
    BOOST_REQUIRE (graph.addConnection (pass2->nodeId, pass2Out, dst2->nodeId, dst2In));
    BOOST_REQUIRE (graph.addConnection (pass2->nodeId, pass2Out, dst3->nodeId, dst3In));

    graph.rebuild();

    // Src1: 2 events, Src2: 3 events
    src1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    src1->addEvent (MidiMessage::noteOff (1, 60), 10);

    src2->addEvent (MidiMessage::noteOn (1, 67, 0.7f), 5);
    src2->addEvent (MidiMessage::noteOn (1, 71, 0.7f), 15);
    src2->addEvent (MidiMessage::noteOff (1, 67), 25);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Dst1 gets src1 only: 2 events
    BOOST_REQUIRE_EQUAL (dst1->getEventCount(), 2);
    // Dst2 gets src1 + src2: 5 events
    BOOST_REQUIRE_EQUAL (dst2->getEventCount(), 5);
    // Dst3 gets src2 only: 3 events
    BOOST_REQUIRE_EQUAL (dst3->getEventCount(), 3);
}

BOOST_AUTO_TEST_CASE (MidiMultiLevelMerging)
{
    // Test hierarchical merging:
    // Src1 --\           /-> FinalDst1
    //         +-> Merge1-
    // Src2 --/           \-> FinalDst2
    //
    // Src3 --\           /-> FinalDst2
    //         +-> Merge2-
    // Src4 --/           \-> FinalDst3

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    class MidiPassNode : public TestNode {
    public:
        MidiPassNode() : TestNode (0, 0, 1, 1) {}

        void render (RenderContext& rc) override
        {
            if (rc.midi.getNumBuffers() >= 2) {
                auto* input = rc.midi.getWriteBuffer (0);
                auto* output = rc.midi.getWriteBuffer (1);
                output->clear();
                output->addEvents (*input, 0, 512, 0);
            }
        }
    };

    auto* src1 = new MidiGeneratorNode();
    auto* src2 = new MidiGeneratorNode();
    auto* src3 = new MidiGeneratorNode();
    auto* src4 = new MidiGeneratorNode();
    auto* merge1 = new MidiPassNode(); // Merges src1 + src2
    auto* merge2 = new MidiPassNode(); // Merges src3 + src4
    auto* finalDst1 = new MidiCaptureNode();
    auto* finalDst2 = new MidiCaptureNode(); // Gets both merge1 + merge2
    auto* finalDst3 = new MidiCaptureNode();

    graph.addNode (src1);
    graph.addNode (src2);
    graph.addNode (src3);
    graph.addNode (src4);
    graph.addNode (merge1);
    graph.addNode (merge2);
    graph.addNode (finalDst1);
    graph.addNode (finalDst2);
    graph.addNode (finalDst3);

    // Connect sources to merge nodes
    BOOST_REQUIRE (graph.addConnection (src1->nodeId, src1->getPortForChannel (PortType::Midi, 0, false), merge1->nodeId, merge1->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (src2->nodeId, src2->getPortForChannel (PortType::Midi, 0, false), merge1->nodeId, merge1->getPortForChannel (PortType::Midi, 0, true)));

    BOOST_REQUIRE (graph.addConnection (src3->nodeId, src3->getPortForChannel (PortType::Midi, 0, false), merge2->nodeId, merge2->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (src4->nodeId, src4->getPortForChannel (PortType::Midi, 0, false), merge2->nodeId, merge2->getPortForChannel (PortType::Midi, 0, true)));

    // Connect merge outputs to final destinations
    BOOST_REQUIRE (graph.addConnection (merge1->nodeId, merge1->getPortForChannel (PortType::Midi, 0, false), finalDst1->nodeId, finalDst1->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (merge1->nodeId, merge1->getPortForChannel (PortType::Midi, 0, false), finalDst2->nodeId, finalDst2->getPortForChannel (PortType::Midi, 0, true)));

    BOOST_REQUIRE (graph.addConnection (merge2->nodeId, merge2->getPortForChannel (PortType::Midi, 0, false), finalDst2->nodeId, finalDst2->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (merge2->nodeId, merge2->getPortForChannel (PortType::Midi, 0, false), finalDst3->nodeId, finalDst3->getPortForChannel (PortType::Midi, 0, true)));

    graph.rebuild();

    // Each source generates different number of events
    src1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    src2->addEvent (MidiMessage::noteOn (1, 62, 0.8f), 5);
    src2->addEvent (MidiMessage::noteOff (1, 62), 10);

    src3->addEvent (MidiMessage::noteOn (1, 64, 0.8f), 15);
    src3->addEvent (MidiMessage::noteOn (1, 67, 0.8f), 20);
    src3->addEvent (MidiMessage::noteOff (1, 64), 25);

    src4->addEvent (MidiMessage::noteOn (1, 69, 0.8f), 30);
    src4->addEvent (MidiMessage::noteOn (1, 71, 0.8f), 35);
    src4->addEvent (MidiMessage::noteOff (1, 69), 40);
    src4->addEvent (MidiMessage::noteOff (1, 71), 45);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify hierarchical merging and distribution
    BOOST_REQUIRE_EQUAL (finalDst1->getEventCount(), 3);  // merge1: src1(1) + src2(2)
    BOOST_REQUIRE_EQUAL (finalDst2->getEventCount(), 10); // merge1(3) + merge2(7)
    BOOST_REQUIRE_EQUAL (finalDst3->getEventCount(), 7);  // merge2: src3(3) + src4(4)
}

BOOST_AUTO_TEST_CASE (MidiIOPlusGenerators)
{
    // Critical test: Graph I/O MIDI input + internal generators -> multiple destinations
    // Tests that external MIDI and generated MIDI can coexist and route correctly

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiIn = new IONode (IONode::midiInputNode);
    auto* gen1 = new MidiGeneratorNode();
    auto* gen2 = new MidiGeneratorNode();
    auto* dst1 = new MidiCaptureNode(); // Gets I/O + gen1
    auto* dst2 = new MidiCaptureNode(); // Gets gen2 only
    auto* dst3 = new MidiCaptureNode(); // Gets all sources

    graph.addNode (midiIn);
    graph.addNode (gen1);
    graph.addNode (gen2);
    graph.addNode (dst1);
    graph.addNode (dst2);
    graph.addNode (dst3);

    const auto midiInOut = midiIn->getPortForChannel (PortType::Midi, 0, false);
    const auto gen1Out = gen1->getPortForChannel (PortType::Midi, 0, false);
    const auto gen2Out = gen2->getPortForChannel (PortType::Midi, 0, false);
    const auto dst1In = dst1->getPortForChannel (PortType::Midi, 0, true);
    const auto dst2In = dst2->getPortForChannel (PortType::Midi, 0, true);
    const auto dst3In = dst3->getPortForChannel (PortType::Midi, 0, true);

    // dst1 <- midiIn + gen1
    BOOST_REQUIRE (graph.addConnection (midiIn->nodeId, midiInOut, dst1->nodeId, dst1In));
    BOOST_REQUIRE (graph.addConnection (gen1->nodeId, gen1Out, dst1->nodeId, dst1In));

    // dst2 <- gen2
    BOOST_REQUIRE (graph.addConnection (gen2->nodeId, gen2Out, dst2->nodeId, dst2In));

    // dst3 <- midiIn + gen1 + gen2
    BOOST_REQUIRE (graph.addConnection (midiIn->nodeId, midiInOut, dst3->nodeId, dst3In));
    BOOST_REQUIRE (graph.addConnection (gen1->nodeId, gen1Out, dst3->nodeId, dst3In));
    BOOST_REQUIRE (graph.addConnection (gen2->nodeId, gen2Out, dst3->nodeId, dst3In));

    graph.rebuild();

    // External MIDI: 4 events
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 48, 0.9f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 52, 0.9f), 10);
    midi.addEvent (MidiMessage::noteOff (1, 48), 20);
    midi.addEvent (MidiMessage::noteOff (1, 52), 30);

    // gen1: 2 events
    gen1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 5);
    gen1->addEvent (MidiMessage::noteOff (1, 60), 15);

    // gen2: 3 events
    gen2->addEvent (MidiMessage::noteOn (1, 67, 0.7f), 8);
    gen2->addEvent (MidiMessage::noteOn (1, 71, 0.7f), 18);
    gen2->addEvent (MidiMessage::noteOff (1, 67), 28);

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify routing with I/O + generators
    BOOST_REQUIRE_EQUAL (dst1->getEventCount(), 6); // midiIn(4) + gen1(2)
    BOOST_REQUIRE_EQUAL (dst2->getEventCount(), 3); // gen2(3) only
    BOOST_REQUIRE_EQUAL (dst3->getEventCount(), 9); // midiIn(4) + gen1(2) + gen2(3)
}

BOOST_AUTO_TEST_CASE (MidiOutputIOUnused)
{
    // Test that unused MIDI output IO node doesn't interfere with signal chain

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiOut = new IONode (IONode::midiOutputNode); // Unused
    auto* gen = new MidiGeneratorNode();
    auto* dst = new MidiCaptureNode();

    graph.addNode (midiOut);
    graph.addNode (gen);
    graph.addNode (dst);

    // Connect gen -> dst only, midiOut unused
    BOOST_REQUIRE (graph.addConnection (gen->nodeId, gen->getPortForChannel (PortType::Midi, 0, false), dst->nodeId, dst->getPortForChannel (PortType::Midi, 0, true)));

    graph.rebuild();

    gen->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    gen->addEvent (MidiMessage::noteOff (1, 60), 10);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify unused MIDI output doesn't interfere
    BOOST_REQUIRE_EQUAL (dst->getEventCount(), 2);
}

BOOST_AUTO_TEST_CASE (MidiOutputIOConnected)
{
    // Test MIDI output IO node with actual connections
    // Gen -> dst (internal capture) AND -> midiOut (external)

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiOut = new IONode (IONode::midiOutputNode);
    auto* gen = new MidiGeneratorNode();
    auto* dst = new MidiCaptureNode();

    graph.addNode (midiOut);
    graph.addNode (gen);
    graph.addNode (dst);

    // Connect gen -> dst (internal) AND gen -> midiOut (external)
    BOOST_REQUIRE (graph.addConnection (gen->nodeId, gen->getPortForChannel (PortType::Midi, 0, false), dst->nodeId, dst->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (gen->nodeId, gen->getPortForChannel (PortType::Midi, 0, false), midiOut->nodeId, midiOut->getPortForChannel (PortType::Midi, 0, true)));

    graph.rebuild();

    gen->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    gen->addEvent (MidiMessage::noteOff (1, 60), 10);
    gen->addEvent (MidiMessage::noteOn (1, 62, 0.8f), 20);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify internal routing still works with MIDI output connected
    BOOST_REQUIRE_EQUAL (dst->getEventCount(), 3);
}

BOOST_AUTO_TEST_CASE (MidiFullIOChain)
{
    // Test full I/O chain with both input and output
    // midiIn -> processor -> midiOut
    // Also test multiple internal destinations don't interfere

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiIn = new IONode (IONode::midiInputNode);
    auto* midiOut = new IONode (IONode::midiOutputNode);
    auto* gen = new MidiGeneratorNode();
    auto* dst1 = new MidiCaptureNode();
    auto* dst2 = new MidiCaptureNode();

    graph.addNode (midiIn);
    graph.addNode (midiOut);
    graph.addNode (gen);
    graph.addNode (dst1);
    graph.addNode (dst2);

    // Complex routing: gen -> {dst1, midiOut}, midiIn -> dst2
    BOOST_REQUIRE (graph.addConnection (gen->nodeId, gen->getPortForChannel (PortType::Midi, 0, false), dst1->nodeId, dst1->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (gen->nodeId, gen->getPortForChannel (PortType::Midi, 0, false), midiOut->nodeId, midiOut->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (midiIn->nodeId, midiIn->getPortForChannel (PortType::Midi, 0, false), dst2->nodeId, dst2->getPortForChannel (PortType::Midi, 0, true)));

    graph.rebuild();

    // External MIDI input: 2 events
    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;
    midi.addEvent (MidiMessage::noteOn (1, 48, 0.9f), 0);
    midi.addEvent (MidiMessage::noteOff (1, 48), 10);

    // Generator: 3 events
    gen->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 5);
    gen->addEvent (MidiMessage::noteOff (1, 60), 15);
    gen->addEvent (MidiMessage::noteOn (1, 62, 0.8f), 25);

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify routing: gen events go to dst1, midiIn events go to dst2
    BOOST_REQUIRE_EQUAL (dst1->getEventCount(), 3); // gen(3)
    BOOST_REQUIRE_EQUAL (dst2->getEventCount(), 2); // midiIn(2)
}

BOOST_AUTO_TEST_CASE (MidiMultipleOutputs)
{
    // Test multiple MIDI output nodes (edge case)
    // Gen1 -> midiOut1, Gen2 -> {midiOut2, dst1}

    PreparedGraph fix;
    GraphNode& graph = fix.graph;

    auto* midiOut1 = new IONode (IONode::midiOutputNode);
    auto* midiOut2 = new IONode (IONode::midiOutputNode);
    auto* gen1 = new MidiGeneratorNode();
    auto* gen2 = new MidiGeneratorNode();
    auto* dst1 = new MidiCaptureNode();

    graph.addNode (midiOut1);
    graph.addNode (midiOut2);
    graph.addNode (gen1);
    graph.addNode (gen2);
    graph.addNode (dst1);

    // Gen1 -> midiOut1 only
    BOOST_REQUIRE (graph.addConnection (gen1->nodeId, gen1->getPortForChannel (PortType::Midi, 0, false), midiOut1->nodeId, midiOut1->getPortForChannel (PortType::Midi, 0, true)));

    // Gen2 -> midiOut2 AND dst1
    BOOST_REQUIRE (graph.addConnection (gen2->nodeId, gen2->getPortForChannel (PortType::Midi, 0, false), midiOut2->nodeId, midiOut2->getPortForChannel (PortType::Midi, 0, true)));
    BOOST_REQUIRE (graph.addConnection (gen2->nodeId, gen2->getPortForChannel (PortType::Midi, 0, false), dst1->nodeId, dst1->getPortForChannel (PortType::Midi, 0, true)));

    graph.rebuild();

    gen1->addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    gen1->addEvent (MidiMessage::noteOff (1, 60), 10);

    gen2->addEvent (MidiMessage::noteOn (1, 62, 0.8f), 5);
    gen2->addEvent (MidiMessage::noteOn (1, 64, 0.8f), 15);
    gen2->addEvent (MidiMessage::noteOff (1, 62), 25);

    AudioSampleBuffer audio (2, 512);
    MidiBuffer midi;

    RenderContext rc (audio, audio, midi, 512);
    graph.render (rc);

    // Verify dst1 only gets gen2 events, not gen1
    BOOST_REQUIRE_EQUAL (dst1->getEventCount(), 3); // gen2(3) only
}

BOOST_AUTO_TEST_SUITE_END()
