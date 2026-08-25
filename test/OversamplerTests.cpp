// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include <element/oversampler.hpp>

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

BOOST_AUTO_TEST_SUITE_END()
