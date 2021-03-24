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
#include "engine/nodes/MidiChannelSplitterNode.h"

namespace Element {

class GraphProcessorTest : public UnitTestBase
{
public:
    GraphProcessorTest() : UnitTestBase ("Graph Processor", "graphProc1", "processor") { }

    void initialise() override
    {
        globals.reset (new Globals());
        globals->getPluginManager().addDefaultFormats();
        globals->getPluginManager().addFormat (new ElementAudioPluginFormat (*globals));
        globals->getPluginManager().setPlayConfig (44100.0, 512);
    }

    void shutdown() override
    {
        globals.reset (nullptr);
    }

    void runTest() override
    {
        if (auto* const plugin = createPluginProcessor())
        {
            GraphProcessor graph;
            graph.prepareToPlay (44100.0, 512);

            beginTest ("adds/removes node");
            NodeObjectPtr node = graph.addNode (plugin);
            MessageManager::getInstance()->runDispatchLoopUntil (10);
            expect (graph.getNumNodes() == 1, "node wasn't added");
            expect (node != nullptr);
            expect (node->getAudioProcessor() == plugin);
            expect (graph.removeNode (node->nodeId), "node wasn't removed");

            graph.releaseResources();
            graph.clear();
        }

        {
            const double sampleRate = 44100.0;
            const int procLatencySamples = 100;
            const double delayComp = 10.0;

            GraphProcessor graph;
            graph.setPlayConfigDetails (0, 2, sampleRate, 512);
            graph.prepareToPlay (44100.0, 512);
            
            
            auto* const plugin1 = createPluginProcessor();
            plugin1->setLatencySamples (procLatencySamples);
            auto* const plugin2 = new Element::GraphProcessor::AudioGraphIOProcessor (
                GraphProcessor::AudioGraphIOProcessor::audioOutputNode);
            
            NodeObjectPtr node1 = graph.addNode (plugin1);
            node1->setDelayCompensation (delayComp);
            beginTest ("node latencies");
            expect (node1->getDelayCompensation() == delayComp);
            expect (node1->getDelayCompensationSamples() == roundToInt (sampleRate * delayComp * 0.001));
            NodeObjectPtr node2 = graph.addNode (plugin2);
            node1->connectAudioTo (node2);
            graph.handleUpdateNowIfNeeded();

            auto nc = graph.getNumConnections();
            auto ls = graph.getLatencySamples();
            beginTest ("graph details");
            expect (graph.getNumConnections() == 2);
            expect (graph.getLatencySamples() == node1->getLatencySamples());
            
            node1 = nullptr; node2 = nullptr;
            graph.releaseResources();
            graph.clear();
        }

        {
            GraphProcessor graph;
            graph.setPlayConfigDetails (0, 2, 44100.0, 512);
            graph.prepareToPlay (44100.0, 512);
           
            NodeObjectPtr midiIn = graph.addNode (new Element::GraphProcessor::AudioGraphIOProcessor (
                GraphProcessor::AudioGraphIOProcessor::midiInputNode));
            NodeObjectPtr midiOut = graph.addNode (new Element::GraphProcessor::AudioGraphIOProcessor (
                GraphProcessor::AudioGraphIOProcessor::midiOutputNode));
            NodeObjectPtr filter = graph.addNode (new MidiChannelSplitterNode());
            graph.handleUpdateNowIfNeeded();

            beginTest ("port/channel mappings");
            expect (filter->getNumPorts() == 17);
            expect (filter->getNumPorts (PortType::Midi, true) == 1);
            expect (filter->getNumPorts (PortType::Midi, false) == 16);
            expect (filter->getPortForChannel (PortType::Midi, 0, true) == 0);
            expect (filter->getPortForChannel (PortType::Midi, 0, false) == 1);
            expect (filter->getPortForChannel (PortType::Midi, 8, false) == 9);
            expect (filter->getChannelPort(0) == 0);
            expect (filter->getChannelPort(1) == 0);
            expect (filter->getChannelPort(9) == 8);

            expect (midiOut->getNumPorts() == 1);
            expect (midiOut->getPortForChannel (PortType::Midi, 0, true) == 0);
            expect (midiOut->getChannelPort(0) == 0);
            
            beginTest ("midi filter connectivity");
            expect (graph.connectChannels (PortType::Midi, midiIn->nodeId, 0, filter->nodeId, 0));
            
            for (int ch = 0; ch < 16; ++ch)
                expect (graph.connectChannels (PortType::Midi, filter->nodeId, ch, midiOut->nodeId, 0));
            
            graph.handleUpdateNowIfNeeded();
            graph.releaseResources();
            graph.clear();
        }
    }

private:
    std::unique_ptr<Globals> globals;
    AudioProcessor* createPluginProcessor()
    {
        auto& plugins (globals->getPluginManager());

        PluginDescription desc;
        desc.pluginFormatName = "Element";
        desc.fileOrIdentifier = "element.volume.stereo";
        String msg;

        return plugins.createAudioPlugin (desc, msg);
    }
};

static GraphProcessorTest sGraphProcessorTest;


class GraphNodeTest : public UnitTestBase
{
public:
    GraphNodeTest (const String& name, 
                   const String& slug = String(),
                   const String& category = "NodeObject")
        : UnitTestBase (name, category, slug) { }

    void initialise() override
    {
        graph.reset (new GraphProcessor());
        graph->prepareToPlay (44100.f, 1024);
    }

    void shutdown() override
    { 
        graph->releaseResources();
        graph.reset (nullptr);
    }

protected:
    std::unique_ptr<GraphProcessor> graph;
};

namespace GraphNodeTests {

class GetMidiInputPort : public GraphNodeTest
{
public:
    GetMidiInputPort() : GraphNodeTest ("Node Midi Ports", "midiPorts") { }
    void runTest() override
    {
       #if JUCE_MAC
        AudioPluginFormatManager plugins;
        plugins.addDefaultFormats();
        PluginDescription desc;
        desc.pluginFormatName = "AudioUnit";
        desc.fileOrIdentifier = "AudioUnit:Synths/aumu,samp,appl";
        String msg;

        if (auto* plugin = plugins.createPluginInstance (desc, 44100.0, 1024, msg).release())
        {
            beginTest ("finds MIDI port");
            NodeObjectPtr node = graph->addNode (plugin);
            expect (13 == node->getMidiInputPort());
        }
       #endif
    }
};

static GetMidiInputPort sGetMidiInputPort;


/** Test nodes can be enabled and disabled */
class EnablementTest : public GraphNodeTest
{
public:
    EnablementTest() : GraphNodeTest ("Node Enablement") { }
    void runTest() override
    {
        checkNode ("audio processor", graph->addNode (new PlaceholderProcessor (2, 2, false, false)));
    }

    void checkNode (const String& testName, NodeObjectPtr node)
    {
        beginTest (testName);
        expect (node->isEnabled());
        node->setEnabled (false);
        expect (! node->isEnabled());
        node->setEnabled (true);
        expect (node->isEnabled());
    }
};

static EnablementTest sEnablementTest;

/** Test nodes get the correct type property */
class GetTypeStringTest : public GraphNodeTest
{
public:
    GetTypeStringTest() : GraphNodeTest ("Node Type") { }
    void runTest() override
    {
        checkNode ("plugin", graph->addNode (new PlaceholderProcessor (2, 2, false, false)), Tags::plugin);
        checkNode ("graph", graph->addNode (new SubGraphProcessor()), Tags::graph);
    }

    void checkNode (const String& testName, NodeObjectPtr node, const Identifier& expectedType)
    {
        beginTest (testName);
        expect (node->getTypeString() == expectedType.toString());
        const Node model (node->getMetadata(), false);
        expect (model.getNodeType() == expectedType);
    }
};

static GetTypeStringTest sGetTypeStringTest;

}

}
