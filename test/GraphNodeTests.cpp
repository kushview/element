#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "graph/GraphNode.h"
#include "Utils.h"

using namespace Element;

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

BOOST_AUTO_TEST_CASE (AddRemove)
{
    PreparedGraph fix;
    GraphNode& graph = fix.graph;
    NodeObjectPtr node = graph.addNode (new TestNode());
    MessageManager::getInstance()->runDispatchLoopUntil (10);
    BOOST_REQUIRE (graph.getNumNodes() == 1);
    BOOST_REQUIRE (node != nullptr);
    BOOST_REQUIRE (graph.removeNode (node->nodeId));
}

BOOST_AUTO_TEST_SUITE_END()
