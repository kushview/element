// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/oversampler.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include <limits>

using namespace element;

BOOST_AUTO_TEST_SUITE (OversamplerTests)

BOOST_AUTO_TEST_CASE (Basics)
{
    Oversampler<float> os;
    BOOST_REQUIRE (os.getNumProcessors() == 0);
    BOOST_REQUIRE (os.getProcessor (0) == nullptr);
    BOOST_REQUIRE (os.getLatencySamples (0) == 0);
    BOOST_REQUIRE (os.getFactor (0) == 1);

    // prepare() records the spec but does not build any chains.
    os.prepare (2, 1024);
    BOOST_REQUIRE (os.getNumProcessors() == 0);
    BOOST_REQUIRE (os.getProcessor (0) == nullptr);

    // Chains are created on demand.
    for (int i = 0; i < 3; ++i) {
        auto* const proc = os.ensureProcessor (i);
        BOOST_REQUIRE (nullptr != proc);
        BOOST_REQUIRE (proc == os.getProcessor (i));
        size_t factora = static_cast<size_t> (std::pow (2.0, (double) (i + 1)));
        size_t factorb = proc->getOversamplingFactor();
        BOOST_REQUIRE_EQUAL (os.getFactor (i), (int) proc->getOversamplingFactor());
        BOOST_REQUIRE_EQUAL (factora, factorb);
        BOOST_REQUIRE (proc->getLatencyInSamples() > 0.f);
        BOOST_REQUIRE (os.getLatencySamples (i) > 0.f);
    }

    // ensureProcessor is idempotent.
    auto* const first = os.getProcessor (1);
    BOOST_REQUIRE (first == os.ensureProcessor (1));

    // Out of range or invalid indexes return nullptr.
    BOOST_REQUIRE (os.ensureProcessor (-1) == nullptr);
    BOOST_REQUIRE (os.ensureProcessor (3) == nullptr);

    // Changing the spec drops stale chains until re-ensured.
    os.prepare (2, 512);
    BOOST_REQUIRE (os.getProcessor (1) == nullptr);
    BOOST_REQUIRE (nullptr != os.ensureProcessor (1));

    os.reset();
}

BOOST_AUTO_TEST_CASE (EnsureWithoutPrepare)
{
    Oversampler<float> os;
    BOOST_REQUIRE (os.ensureProcessor (0) == nullptr);
}

BOOST_AUTO_TEST_CASE (ProcessorRejectsUnsupportedFactors)
{
    PreparedGraph graph (48000, 128);
    auto* node = new TestNode (2, 2, 0, 0);
    graph.graph.addNode (node);
    for (int valid : { 1, 2, 4, 8, 1 })
    {
        node->setOversamplingFactor (valid);
        BOOST_REQUIRE_EQUAL (node->getOversamplingFactor(), valid);
        const auto latency = node->getLatencySamples();
        const auto enabled = node->isEnabled();
        for (int invalid : { 0, -1, std::numeric_limits<int>::min(), 3, 5, 7, 9, 16, std::numeric_limits<int>::max() })
        {
            node->setOversamplingFactor (invalid);
            BOOST_CHECK_EQUAL (node->getOversamplingFactor(), valid);
            BOOST_CHECK_EQUAL (node->getLatencySamples(), latency);
            BOOST_CHECK_EQUAL (node->isEnabled(), enabled);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
