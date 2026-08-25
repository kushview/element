// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/node.h>
#include <element/node.hpp>
#include <element/nodefactory.hpp>
#include <element/plugins.hpp>
#include <element/tags.hpp>

#include "engine/graphmanager.hpp"
#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"
#include "fixture/TestNode.h"
#include "testutil.hpp"

using namespace element;

namespace {

constexpr const char* countingNodeID = "test.sessionLoadCounter";

/** A TestNode which counts prepares and state restores, so tests can assert
    each happens exactly once during a session load. */
struct CountingNode : public TestNode
{
    static std::atomic<int> prepares;
    static std::atomic<int> restores;

    static void resetCounters()
    {
        prepares = 0;
        restores = 0;
    }

    void prepareToRender (double newSampleRate, int newBlockSize) override
    {
        ++prepares;
        TestNode::prepareToRender (newSampleRate, newBlockSize);
    }

    void setState (const void*, int) override { ++restores; }

    void getPluginDescription (juce::PluginDescription& desc) const override
    {
        desc.pluginFormatName = EL_NODE_FORMAT_NAME;
        desc.fileOrIdentifier = countingNodeID;
        desc.name = "Session Load Counter";
        desc.manufacturerName = "Element";
    }
};

std::atomic<int> CountingNode::prepares { 0 };
std::atomic<int> CountingNode::restores { 0 };

struct CountingNodeProvider : public NodeProvider
{
    juce::String format() const override { return EL_NODE_FORMAT_NAME; }

    Processor* create (const juce::String& identifier) override
    {
        return identifier == countingNodeID ? new CountingNode() : nullptr;
    }

    juce::StringArray findTypes (const juce::FileSearchPath&, bool, bool) override
    {
        return { countingNodeID };
    }
};

element::Context* countingProviderContext = nullptr;

void registerCountingProvider()
{
    auto* const ctx = element::test::context();
    if (countingProviderContext == ctx)
        return;
    ctx->plugins().getNodeFactory().add (new CountingNodeProvider());
    countingProviderContext = ctx;
}

juce::ValueTree makeCountingNodeData (juce::int64 nodeId)
{
    juce::ValueTree data (types::Node);
    juce::MemoryBlock state;
    state.append ("state", 5);
    data.setProperty (tags::id, nodeId, nullptr)
        .setProperty (tags::type, "plugin", nullptr)
        .setProperty (tags::format, EL_NODE_FORMAT_NAME, nullptr)
        .setProperty (tags::identifier, countingNodeID, nullptr)
        .setProperty (tags::name, "Counter " + juce::String (nodeId), nullptr)
        .setProperty (tags::state, state.toBase64Encoding(), nullptr);
    return data;
}

Node makeGraphModel (const int numCountingNodes, const int numDuplicateIONodes = 0)
{
    auto graph = Node::createDefaultGraph ("bench");
    auto nodes = graph.getNodesValueTree();

    juce::int64 nodeId = 100;
    for (int i = 0; i < numCountingNodes; ++i)
        nodes.addChild (makeCountingNodeData (nodeId++), -1, nullptr);

    // Simulates a session corrupted by the historical IO node duplication bug:
    // extra "Audio Out" nodes which the IONodeEnforcer repairs at load time.
    for (int i = 0; i < numDuplicateIONodes; ++i)
    {
        juce::ValueTree dupe (types::Node);
        dupe.setProperty (tags::id, nodeId++, nullptr)
            .setProperty (tags::type, "plugin", nullptr)
            .setProperty (tags::format, "Internal", nullptr)
            .setProperty (tags::identifier, "audio.output", nullptr)
            .setProperty (tags::name, "Audio Out", nullptr);
        nodes.addChild (dupe, -1, nullptr);
    }

    return graph;
}

/** Like the PreparedGraph fixture, but heap-allocates the graph: GraphManager
    stores refcounting pointers to it (via NodeModelUpdater), so it must not
    live on the stack. */
struct PreparedManagedGraph
{
    ProcessorPtr keep;
    GraphNode& graph;

    PreparedManagedGraph()
        : keep (new GraphNode (*element::test::context())),
          graph (*static_cast<GraphNode*> (keep.get()))
    {
        graph.prepareToRender (44100.0, 512);
    }

    ~PreparedManagedGraph()
    {
        graph.releaseResources();
        graph.clear();
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE (SessionLoadBenchTests)

BOOST_AUTO_TEST_CASE (SinglePrepareAndRestorePerNode)
{
    registerCountingProvider();
    CountingNode::resetCounters();

    PreparedManagedGraph fix;
    auto& graph = fix.graph;

    const int numNodes = 8;
    auto model = makeGraphModel (numNodes);

    int rebuilds = 0;
    graph.renderingSequenceChanged.connect ([&rebuilds]() { ++rebuilds; });

    const auto startTicks = juce::Time::getHighResolutionTicks();
    GraphManager manager (graph, test::context()->plugins());
    manager.setNodeModel (model);
    const auto elapsed = juce::Time::highResolutionTicksToSeconds (
        juce::Time::getHighResolutionTicks() - startTicks);
    BOOST_TEST_MESSAGE ("setNodeModel: " << juce::String (elapsed * 1000.0, 2) << " ms");

    BOOST_REQUIRE_EQUAL (model.getNodesValueTree().getNumChildren(), graph.getNumNodes());

    // Each node prepares exactly once and restores state exactly once.
    BOOST_REQUIRE_EQUAL (CountingNode::prepares.load(), numNodes);
    BOOST_REQUIRE_EQUAL (CountingNode::restores.load(), numNodes);

    // A clean load defers every rebuild to one coalesced async update.
    BOOST_REQUIRE_EQUAL (rebuilds, 0);
    graph.rebuild();
    BOOST_REQUIRE_EQUAL (rebuilds, 1);
}

BOOST_AUTO_TEST_CASE (DuplicateIONodeRepair)
{
    registerCountingProvider();
    CountingNode::resetCounters();

    PreparedManagedGraph fix;
    auto& graph = fix.graph;

    const int numNodes = 4;
    const int numDupes = 3;
    auto model = makeGraphModel (numNodes, numDupes);

    int rebuilds = 0;
    graph.renderingSequenceChanged.connect ([&rebuilds]() { ++rebuilds; });

    GraphManager manager (graph, test::context()->plugins());
    manager.setNodeModel (model);

    // Duplicates were repaired: model and engine agree, one of each IO type.
    BOOST_REQUIRE_EQUAL (model.getNodesValueTree().getNumChildren(), graph.getNumNodes());
    int audioOuts = 0;
    for (int i = 0; i < graph.getNumNodes(); ++i)
        if (auto* io = dynamic_cast<IONode*> (graph.getNode (i)))
            if (io->getType() == IONode::audioOutputNode)
                ++audioOuts;
    BOOST_REQUIRE_EQUAL (audioOuts, 1);

    // The whole repair batch costs a single synchronous rebuild.
    BOOST_REQUIRE_EQUAL (rebuilds, 1);

    // Repair must not disturb the other nodes.
    BOOST_REQUIRE_EQUAL (CountingNode::prepares.load(), numNodes);
    BOOST_REQUIRE_EQUAL (CountingNode::restores.load(), numNodes);
}

BOOST_AUTO_TEST_SUITE_END()
