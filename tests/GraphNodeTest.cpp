/*
    GraphNodeTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"

namespace Element {

class GraphProcessorTest : public UnitTest {
public:
    GraphProcessorTest() : UnitTest ("Graph Processor", "graph") { }

    virtual void initialise() override
    {
        globals.reset (new Globals());
        globals->getPluginManager().addDefaultFormats();
    }

    virtual void shutdown() override
    {
        globals.reset (nullptr);
    }

    virtual void runTest() override
    {
        if (auto* plugin = createPluginProcessor())
        {
            beginTest ("Channel/Port Mapping");
            GraphProcessor graph;
            GraphNodePtr node = graph.addNode (plugin);
            expect (13 == node->getMidiInputPort());
        }
    }

private:
    std::unique_ptr<Globals> globals;
    AudioPluginFormatManager plugins;
    AudioProcessor* createPluginProcessor()
    {
        PluginDescription desc;
        desc.pluginFormatName = "AudioUnit";
        desc.fileOrIdentifier = "AudioUnit:Synths/aumu,samp,appl";
        String msg;
        return plugins.createPluginInstance (desc, 44100.0, 1024, msg);
    }
};

static GraphProcessorTest sGraphProcessorTest;


class GraphNodeTest : public UnitTest {
public:
    GraphNodeTest() : UnitTest ("Graph Node", "graph") { }

    virtual void initialise() { }
    virtual void shutdown() { }

    virtual void runTest()
    {
        AudioPluginFormatManager plugins;
        plugins.addDefaultFormats();
        PluginDescription desc;
        desc.pluginFormatName = "AudioUnit";
        desc.fileOrIdentifier = "AudioUnit:Synths/aumu,samp,appl";
        String msg;
        if (auto* plugin = plugins.createPluginInstance (desc, 44100.0, 1024, msg))
        {
            beginTest ("Channel/Port Mapping");
            GraphProcessor graph;
            GraphNodePtr node = graph.addNode (plugin);
            expect (13 == node->getMidiInputPort());
        }
    }
};

static GraphNodeTest sGraphNodeTest;

}
