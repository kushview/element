/*
    GraphNodeTest.cpp - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#include "Tests.h"
#include "engine/GraphProcessor.h"

namespace Element {
class GraphNodeTest : public UnitTest {
public:
    GraphNodeTest() : UnitTest ("Graph Node", "Graphing") { }

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
            DBG("NUM PORTS: " << (int) node->getNumPorts());
            DBG("NUM audio in: " << node->getNumAudioInputs());
            DBG("NUM audio out: " << node->getNumAudioOutputs());
            DBG("MIDI in PORT: " << (int) node->getMidiInputPort());
            DBG("MIDI out PORT: " << (int) node->getMidiOutputPort());
            for (uint32 port = 0; port < node->getNumPorts(); ++port)
            {
                const auto type (node->getPortType (port));
                DBG("index: " << (int)port << " type: " << type.getName());
            }
            
            expect (13 == node->getMidiInputPort());
        }
    }
};

static GraphNodeTest sGraphNodeTest;

}
