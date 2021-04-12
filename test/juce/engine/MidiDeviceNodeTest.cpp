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

#if 0

#include "Tests.h"
#include "controllers/GraphManager.h"
#include "engine/nodes/MidiDeviceProcessor.h"

#include "engine/nodes/AudioProcessorNode.h"

namespace Element {

#if ! defined (EL_FREE)

class MidiInputDeviceNodeTest : public UnitTestBase
{
public:
    MidiInputDeviceNodeTest()
        : UnitTestBase ("Midi Input Device Node", "nodes", "midiInputDevice") { }

    void initialise() override
    {
        initializeWorld();
        graph.reset (new GraphProcessor());
        graph->prepareToPlay (44100.f, 1024);
    }

    void shutdown() override
    {
        graph->releaseResources();
        graph.reset (nullptr);
        shutdownWorld();
    }

    void runTest() override
    {
        if (MidiInput::getAvailableDevices().isEmpty())
            return;

        beginTest ("Processor Channels");
        std::unique_ptr<MidiDeviceProcessor> proc;
        proc.reset (new MidiDeviceProcessor (true, getWorld().getMidiEngine()));
        expect (proc->getTotalNumInputChannels() == 0 &&
                proc->getTotalNumOutputChannels() == 0 &&
                proc->acceptsMidi() == false &&
                proc->producesMidi() == true);

        beginTest ("Node Ports");
        NodeObjectPtr node = graph->addNode (proc.release(), 0);
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
        for (int i = 0; i < controller->getNumNodes(); ++i)
        {
            auto object = controller->getNode (i);
            if (nullptr != object->processor<MidiDeviceProcessor>())
            {
                node = object;
                break;
            }
        }
        expect (node != nullptr, "Didn't find NodeObjectPtr");
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

#endif

}
#endif
