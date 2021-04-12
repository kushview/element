#include <boost/test/unit_test.hpp>
#include "graph/GraphNode.h"
#include "Utils.h"

using namespace Element;

class TestNode : public NodeObject
{
public:
    TestNode() : NodeObject (0) {}

    TestNode (int audioIns, int audioOuts, int midiIns, int midiOuts) 
        : NodeObject (0),
          numAudioIns (audioIns),
          numAudioOuts (audioOuts),
          numMidiIns (midiIns),
          numMidiOuts (midiOuts)
    {
        TestNode::refreshPorts();
    }

    void prepareToRender (double newSampleRate, int newBlockSize) override
    {
        sampleRate = newSampleRate;
        bufferSize = newBlockSize;
    }

    void releaseResources() override
    {

    }
    
    bool wantsMidiPipe() const override { return true; }

    void render (AudioSampleBuffer&, MidiPipe&) override { }

    void renderBypassed (AudioSampleBuffer&, MidiPipe&) override {}

    int getNumPrograms() const override { return 1; }
    int getCurrentProgram() const override { return 0; }
    const String getProgramName (int index) const override { return "program"; }
    void setCurrentProgram (int index) override {}

    void getState (MemoryBlock&) override {}
    void setState (const void*, int sizeInBytes) override {}
    
    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = "element.testNode";
        desc.manufacturerName = "Element";
    }

    virtual void refreshPorts() override
    {
        PortList newPorts;
        uint32 port = 0;
        for (int c = 0; c < numAudioIns; c++)
        {
            newPorts.add (PortType::Audio, port++, c,
                       String ("audio_in_") + String (c + 1),  
                       String ("In ") + String (c + 1), 
                       true);
        }

        for (int c = 0; c < numMidiIns; c++)
        {
            newPorts.add (PortType::Midi, port++, c,
                       String ("midi_in_") + String (c + 1),  
                       String ("MIDI In ") + String (c + 1), 
                       true);
        }

        for (int c = 0; c < numAudioOuts; c++)
        {
            newPorts.add (PortType::Audio, port++, c,
                       String ("audio_out_") + String (c + 1),  
                       String ("Out ") + String (c + 1), 
                       false);
        }

        for (int c = 0; c < numMidiOuts; c++)
        {
            newPorts.add (PortType::Midi, port++, c,
                       String ("midi_out_") + String (c + 1),  
                       String ("MIDI Out ") + String (c + 1), 
                       false);
        }

        setPorts (newPorts);
    }

protected:
    int numAudioIns     = 2,
        numAudioOuts    = 2,
        numMidiIns      = 1,
        numMidiOuts     = 1;

    bool prepared;
    double sampleRate;
    int bufferSize;

    void initialize() override {}
};

BOOST_AUTO_TEST_SUITE (GraphNodeTests)

BOOST_AUTO_TEST_CASE (IO)
{
    GraphNode graph;
    BOOST_REQUIRE_EQUAL (graph.getNumAudioInputs(), 2);
    BOOST_REQUIRE_EQUAL (graph.getNumAudioOutputs(), 2);
    graph.clear();
}

BOOST_AUTO_TEST_CASE (Connections)
{
    GraphNode graph;
    {
        auto* node1 = new TestNode();
        graph.addNode (node1);
        BOOST_REQUIRE ((void*)node1 == (void*)graph.getNodeForId (node1->nodeId));

        auto* node2 = new TestNode();
        graph.addNode (node2);
        BOOST_REQUIRE ((void*)node2 == (void*)graph.getNodeForId (node2->nodeId));

        BOOST_REQUIRE_EQUAL (graph.getNumNodes(), 2);
        BOOST_REQUIRE_EQUAL (graph.getNumConnections(), 0);
        const auto srcAudioPort = node1->getPortForChannel (PortType::Audio, 0, false);
        const auto srcMidiPort  = node1->getPortForChannel (PortType::Midi,  0, false);
        const auto dstAudioPort = node2->getPortForChannel (PortType::Audio, 0, true);
        const auto dstMidiPort  = node2->getPortForChannel (PortType::Midi,  0, true);

        BOOST_REQUIRE (graph.canConnect (node1->nodeId, srcAudioPort, node2->nodeId, dstAudioPort));
        BOOST_REQUIRE (graph.canConnect (node1->nodeId, srcMidiPort,  node2->nodeId, dstMidiPort));
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

BOOST_AUTO_TEST_SUITE_END()
