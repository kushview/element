// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/processor.hpp>

#include "engine/graphnode.hpp"
#include "fixture/TestNode.h"
#include "testutil.hpp"

using namespace element;

namespace {
struct LatencyNode : public TestNode
{
    using TestNode::setLatencySamples;
};
} // namespace

BOOST_AUTO_TEST_SUITE (LatencyClampTests)

BOOST_AUTO_TEST_CASE (LatencySamplesAreClamped)
{
    LatencyNode node;
    node.setLatencySamples (512);
    BOOST_REQUIRE_EQUAL (node.getLatencySamples(), 512);

    node.setLatencySamples (-100);
    BOOST_REQUIRE_EQUAL (node.getLatencySamples(), 0);

    node.setLatencySamples (std::numeric_limits<int>::max());
    BOOST_REQUIRE_EQUAL (node.getLatencySamples(), Processor::maxLatencySamples);
}

BOOST_AUTO_TEST_CASE (DelayCompensationSurvivesRestoreBeforePrepare)
{
    // GraphManager-style refcounting means the graph must live on the heap.
    ProcessorPtr keep (new GraphNode (*element::test::context()));
    auto& graph = *static_cast<GraphNode*> (keep.get());

    auto* node = graph.addNode (new TestNode (2, 2, 0, 0));
    BOOST_REQUIRE (node != nullptr);

    // State is restored before the node is prepared during a session load,
    // so the sample rate is unknown at this point.
    node->setDelayCompensation (10.0);
    BOOST_REQUIRE_EQUAL (node->getDelayCompensation(), 10.0);
    BOOST_REQUIRE_EQUAL (node->getDelayCompensationSamples(), 0);

    graph.prepareToRender (48000.0, 512);
    BOOST_REQUIRE_EQUAL (node->getDelayCompensationSamples(), 480);

    node->setDelayCompensation (1.0e9);
    BOOST_REQUIRE_EQUAL (node->getDelayCompensationSamples(), Processor::maxLatencySamples);

    graph.releaseResources();
    graph.clear();
}

BOOST_AUTO_TEST_SUITE_END()
