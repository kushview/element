#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/ionode.hpp"
#include "engine/nodeobject.hpp"

using namespace element;

BOOST_AUTO_TEST_SUITE (NodeObjectTests)

BOOST_AUTO_TEST_CASE (DelayCompensation)
{
    const double sampleRate = 44100.0;
    const int procLatencySamples = 100;
    const double delayComp = 10.0;

    PreparedGraph fix;
    GraphNode& graph = fix.graph;
    
    NodeObjectPtr node1 = graph.addNode (new TestNode());
    node1->setDelayCompensation (delayComp);
    BOOST_REQUIRE (node1->getDelayCompensation() == delayComp);
    BOOST_REQUIRE (node1->getDelayCompensationSamples() == roundToInt (sampleRate * delayComp * 0.001));
    
    NodeObjectPtr node2 = graph.addNode (new IONode (IONode::audioOutputNode));
    node1->connectAudioTo (node2);
    MessageManager::getInstance()->runDispatchLoopUntil (14);
    BOOST_REQUIRE (graph.getNumConnections() == 2);
    BOOST_REQUIRE (graph.getLatencySamples() == node1->getLatencySamples());
    
    node1 = nullptr; 
    node2 = nullptr;
}

BOOST_AUTO_TEST_CASE (Enablement)
{
    PreparedGraph fix;
    NodeObjectPtr node = fix.graph.addNode (new TestNode());

    BOOST_REQUIRE (node->isEnabled());
    node->setEnabled (false);
    BOOST_REQUIRE (! node->isEnabled());
    node->setEnabled (true);
    BOOST_REQUIRE (node->isEnabled());

    node = nullptr;
}

BOOST_AUTO_TEST_CASE (PortChannelMapping)
{
    PreparedGraph fix;
    GraphNode& graph = fix.graph;
    graph.setRenderDetails (44100.0, 512);
    graph.prepareToRender (44100.0, 512);
    
    NodeObjectPtr midiIn = graph.addNode (new element::IONode (
        IONode::midiInputNode));
    NodeObjectPtr midiOut = graph.addNode (new element::IONode (
        IONode::midiOutputNode));
    NodeObjectPtr filter = graph.addNode (new TestNode (0,0,1, 16));
    MessageManager::getInstance()->runDispatchLoopUntil (14);

    BOOST_REQUIRE (filter->getNumPorts() == 17);
    BOOST_REQUIRE (filter->getNumPorts (PortType::Midi, true) == 1);
    BOOST_REQUIRE (filter->getNumPorts (PortType::Midi, false) == 16);
    BOOST_REQUIRE (filter->getPortForChannel (PortType::Midi, 0, true) == 0);
    BOOST_REQUIRE (filter->getPortForChannel (PortType::Midi, 0, false) == 1);
    BOOST_REQUIRE (filter->getPortForChannel (PortType::Midi, 8, false) == 9);
    BOOST_REQUIRE (filter->getChannelPort(0) == 0);
    BOOST_REQUIRE (filter->getChannelPort(1) == 0);
    BOOST_REQUIRE (filter->getChannelPort(9) == 8);

    BOOST_REQUIRE (midiOut->getNumPorts() == 1);
    BOOST_REQUIRE (midiOut->getPortForChannel (PortType::Midi, 0, true) == 0);
    BOOST_REQUIRE (midiOut->getChannelPort(0) == 0);
    
    BOOST_REQUIRE (graph.connectChannels (PortType::Midi, midiIn->nodeId, 0, filter->nodeId, 0));
    for (int ch = 0; ch < 16; ++ch)
        BOOST_REQUIRE (graph.connectChannels (PortType::Midi, filter->nodeId, ch, midiOut->nodeId, 0));
    
    MessageManager::getInstance()->runDispatchLoopUntil (14);
}

BOOST_AUTO_TEST_SUITE_END()
