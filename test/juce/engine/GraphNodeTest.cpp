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
#include "engine/nodes/MidiChannelSplitterNode.h"

namespace Element {

class GraphNodeTest : public UnitTestBase
{
public:
    GraphNodeTest (const String& name, 
                   const String& slug = String(),
                   const String& category = "NodeObject")
        : UnitTestBase (name, category, slug) { }

    void initialise() override
    {
        graph.reset (new GraphNode());
        graph->prepareToRender (44100.f, 1024);
    }

    void shutdown() override
    { 
        graph->releaseResources();
        graph.reset (nullptr);
    }

protected:
    std::unique_ptr<GraphNode> graph;
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

/** Test nodes get the correct type property */
class GetTypeStringTest : public GraphNodeTest
{
public:
    GetTypeStringTest() : GraphNodeTest ("Node Type") { }
    void runTest() override
    {
        // checkNode ("plugin", graph->addNode (new PlaceholderProcessor (2, 2, false, false)), Tags::plugin);
        // checkNode ("graph", graph->addNode (new SubGraphProcessor()), Tags::graph);
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
