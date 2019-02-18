/*
    GraphNodeTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"
#include "controllers/GraphManager.h"
#include "engine/MidiDeviceProcessor.h"
#include "engine/nodes/AudioProcessorNode.h"

namespace Element {

class MidiInputDeviceNodeTest : public UnitTestBase
{
public:
    MidiInputDeviceNodeTest()
        : UnitTestBase ("Midi Input Device Node", "nodes", "midiInputDevice") { }

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

    void runTest() override
    {
        beginTest ("Processor Channels");
        std::unique_ptr<MidiDeviceProcessor> proc;
        proc.reset (new MidiDeviceProcessor (true));
        expect (proc->getTotalNumInputChannels() == 0 &&
                proc->getTotalNumOutputChannels() == 0 &&
                proc->acceptsMidi() == false &&
                proc->producesMidi() == true);

        beginTest ("Node Ports");
        GraphNodePtr node = graph->addNode (proc.release(), 0);
        expect (node->getNumPorts() == 1, "Wrong number of ports");
        node = nullptr;
        graph->clear();
        runDispatchLoop(10);

        beginTest ("Add to Graph Controller");
        initializeWorld();
        std::unique_ptr<GraphManager> controller;
        controller.reset (new GraphManager (*graph, getWorld().getPluginManager()));
        const auto graphFile = getDataDir().getChildFile ("Graphs/IAC Bus 1 Graph.elg");
        jassert(graphFile.existsAsFile());
        Node model (Node::parse (graphFile), false);
        controller->setNodeModel (model);
        for (int i = 0; i < controller->getNumFilters(); ++i)
        {
            auto object = controller->getNode (i);
            if (nullptr != object->processor<MidiDeviceProcessor>())
            {
                node = object;
                break;
            }
        }
        expect (node != nullptr, "Didn't find GraphNodePtr");
        if (node != nullptr)
        {
            expect (node->getNumAudioInputs() == 0, "Audio Ins non-zero");
            expect (node->getNumAudioOutputs() == 0, "Audio Outs non-zero");
            expect (node->getNumPorts (PortType::Midi, true) == 0, "MIDI in non-zero");
            expect (node->getNumPorts (PortType::Midi, false) == 1, "MIDI out not one");
        }

        controller.reset (nullptr);
        shutdownWorld();
    }

protected:
    std::unique_ptr<GraphProcessor> graph;
};

static MidiInputDeviceNodeTest sMidiInputDeviceTest;

}