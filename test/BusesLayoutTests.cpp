// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>

#include "engine/graphnode.hpp"
#include "nodes/audioprocessor.hpp"
#include "fixture/TestNode.h"

using namespace juce;
using namespace element;

namespace {

/** A minimal AudioProcessor with configurable buses and parameters, used to
    reproduce bus-layout changes on plugin nodes without a real plugin. */
class BusLayoutTestProcessor : public AudioProcessor {
public:
    BusLayoutTestProcessor()
    {
        setPlayConfigDetails (8, 2, 44100.0, 512);
    }

    const String getName() const override { return "BusLayoutTestProcessor"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override {}
    void processBlock (AudioBuffer<double>&, MidiBuffer&) override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
    bool hasEditor() const override { return false; }
    AudioProcessorEditor* createEditor() override { return nullptr; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool silenceInProducesSilenceOut() const override { return true; }
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const String getProgramName (int) override { return "default"; }
    void changeProgramName (int, const String&) override {}

    using AudioProcessor::setBusesLayoutWithoutEnabling;
};

} // namespace

BOOST_AUTO_TEST_SUITE (BusesLayoutTests)

/** Reproduces the crash when an input port configuration change shrinks the
    plugin's channel count while connections to the old ports still exist.
    EngineService::changeBusesLayout rebuilds the rendering sequence before
    removing now-illegal connections, so stale arcs feed the GraphBuilder. */
BOOST_AUTO_TEST_CASE (ShrinkInputsBuildsSequenceSafely)
{
    element::Context context;
    GraphNode graph (context);
    graph.setNumPorts (PortType::Audio, 2, true, false);
    graph.setNumPorts (PortType::Audio, 2, false, false);
    graph.setNumPorts (PortType::Midi, 1, true, false);
    graph.setNumPorts (PortType::Midi, 1, false, false);

    auto* proc = new BusLayoutTestProcessor();
    proc->refreshParameterList();
    for (int i = 0; i < 8; ++i)
        proc->addParameter (new AudioParameterFloat ({ "p" + String (i) }, "Param " + String (i), 0.f, 1.f, 0.f));

    ProcessorPtr node = graph.addNode (new AudioProcessorNode (proc));
    BOOST_REQUIRE (node != nullptr);

    graph.prepareToRender (44100.0, 512);

    // Port order for AudioProcessorNode: [audio ins, audio outs, controls, midi]
    // 8 ins + 2 outs + 8 params = 18 ports: idx 0..7 in; 8,9 out; 10..17 control
    BOOST_REQUIRE (node->getNumPorts (PortType::Audio, true) == 8);
    BOOST_REQUIRE (node->getNumPorts (PortType::Audio, false) == 2);
    BOOST_REQUIRE (node->getNumPorts (PortType::Control, true) == 8);

    // Connect a source node's audio out to the plugin's LAST audio input
    // (port 7). Shrinking 8 ins -> 2 renumbers port 7 into the Control range,
    // so this arc becomes stale in the worst possible way.
    auto* srcNode = new TestNode (2, 1, 1, 1);
    srcNode->refreshPorts();
    ProcessorPtr src = graph.addNode (srcNode);

    const auto srcPort = src->getPortForChannel (PortType::Audio, 0, false);
    const auto dstPort = node->getPortForChannel (PortType::Audio, 7, true);
    BOOST_REQUIRE (graph.addConnection (src->nodeId, srcPort, node->nodeId, dstPort));

    // Shrink the plugin to 1-in/1-out, exactly as the I/O configuration window does.
    AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (AudioChannelSet::mono());
    layout.outputBuses.add (AudioChannelSet::mono());
    proc->setBusesLayoutWithoutEnabling (layout);

    // Mirror EngineService::changeBusesLayout exactly: after port renumbering,
    // release + re-prepare the graph, which rebuilds the rendering sequence
    // synchronously BEFORE removeIllegalConnections() runs. A stale arc into
    // audio_in_8 now points at a Control port; GraphBuilder builds
    // BindParameterOp with a null source parameter and crashes in its ctor.
    node->refreshPorts();
    graph.releaseResources();
    graph.prepareToRender (44100.0, 512);

    BOOST_CHECK (true); // reaching here means no crash
}

BOOST_AUTO_TEST_SUITE_END()