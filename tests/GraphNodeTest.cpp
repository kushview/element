/*
    GraphNodeTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
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
            GraphNodePtr node = graph.addNode (plugin);
            MessageManager::getInstance()->runDispatchLoopUntil (10);
            expect (graph.getNumNodes() == 1, "node wasn't added");
            expect (node != nullptr);
            expect (node->getAudioProcessor() == plugin);
            expect (graph.removeNode (node->nodeId), "node wasn't removed");

            graph.releaseResources();
            graph.clear();
        }

        {
            GraphProcessor graph;
            graph.setPlayConfigDetails (0, 2, 44100.0, 512);
            graph.prepareToPlay (44100.0, 512);
            
            beginTest ("audio processor");
            auto* const plugin1 = createPluginProcessor();
            plugin1->setLatencySamples (100);
            auto* const plugin2 = new Element::GraphProcessor::AudioGraphIOProcessor (
                GraphProcessor::AudioGraphIOProcessor::audioOutputNode);
            
            GraphNodePtr node1 = graph.addNode (plugin1);
            GraphNodePtr node2 = graph.addNode (plugin2);
            node1->connectAudioTo (node2);
            for (int i = 0; i < 3; ++i)
                MessageManager::getInstance()->runDispatchLoopUntil (15);

            auto nc = graph.getNumConnections();
            auto ls = graph.getLatencySamples();
            expect (graph.getNumConnections() == 2);
            expect (graph.getLatencySamples() == 100);
            
            node1 = nullptr; node2 = nullptr;
            graph.releaseResources();
            graph.clear();
        }

        {
            GraphProcessor graph;
            graph.setPlayConfigDetails (0, 2, 44100.0, 512);
            graph.prepareToPlay (44100.0, 512);
            
            beginTest ("midi filter connectivity");
           
            GraphNodePtr midiIn = graph.addNode (new Element::GraphProcessor::AudioGraphIOProcessor (
                GraphProcessor::AudioGraphIOProcessor::midiInputNode));
            GraphNodePtr midiOut = graph.addNode (new Element::GraphProcessor::AudioGraphIOProcessor (
                GraphProcessor::AudioGraphIOProcessor::midiInputNode)); 
            GraphNodePtr filter = graph.addNode (new MidiChannelSplitterNode());
            for (int i = 0; i < 2; ++i)
                MessageManager::getInstance()->runDispatchLoopUntil (10);

            expect (graph.connectChannels (PortType::Midi, midiIn->nodeId, 0, filter->nodeId, 0));
            expect (filter->getNumPorts (PortType::Midi, false) == 16);
            for (int ch = 0; ch < 16; ++ch)
                expect (graph.connectChannels (PortType::Midi, filter->nodeId, ch, midiOut->nodeId, 0));
            
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


class GraphNodeTest : public UnitTestBase {
public:
    GraphNodeTest() : UnitTestBase ("Graph Node", "graphs", "node") { }

    void initialise() override { }
    void shutdown() override { }

    void runTest() override
    {
        AudioPluginFormatManager plugins;
        plugins.addDefaultFormats();
        PluginDescription desc;
        desc.pluginFormatName = "AudioUnit";
        desc.fileOrIdentifier = "AudioUnit:Synths/aumu,samp,appl";
        String msg;
        if (auto* plugin = plugins.createPluginInstance (desc, 44100.0, 1024, msg))
        {
            beginTest ("finds MIDI port");
            GraphProcessor graph;
            GraphNodePtr node = graph.addNode (plugin);
            expect (13 == node->getMidiInputPort());
        }
    }
};

static GraphNodeTest sGraphNodeTest;

}
