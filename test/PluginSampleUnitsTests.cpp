// SPDX-FileCopyrightText: 2026 Kushview, LLC
// Author: jiri-god <318975419+jiri-god@users.noreply.github.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/ionode.hpp"
#include "nodes/audioprocessor.hpp"
#include "nodes/eqfilter.hpp"
#include <limits>
#include <algorithm>
using namespace element;
using namespace juce;
namespace {
struct Probe : EQFilterProcessor {
    void prepareToPlay (double rate, int block) override
    {
        EQFilterProcessor::prepareToPlay (rate, block);
        setLatencySamples (257);
    }
};
struct Head : AudioPlayHead {
    Optional<int64_t> samples { -48000 };
    Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        if (samples)
            p.setTimeInSamples (*samples);
        p.setTimeInSeconds (-1);
        p.setPpqPosition (3);
        p.setBpm (120);
        return p;
    }
};
} // namespace
BOOST_AUTO_TEST_SUITE (PluginSampleUnitsTests)
BOOST_AUTO_TEST_CASE (OversamplingUsesPluginSampleUnitsAndBaseRateLatency)
{
    PreparedGraph f (48000, 128);
    Head head;
    f.graph.setPlayHead (&head);
    auto* proc = new Probe;
    auto* node = new AudioProcessorNode (proc);
    f.graph.addNode (node);
    for (int factor : { 1, 2, 4, 1 }) {
        node->setOversamplingFactor (factor);
        BOOST_CHECK_EQUAL (proc->getSampleRate(), 48000 * factor);
        const auto pos = proc->getPlayHead()->getPosition();
        BOOST_REQUIRE (pos.hasValue());
        BOOST_CHECK_EQUAL (*pos->getTimeInSamples(), -48000 * factor);
        BOOST_CHECK_EQUAL (*pos->getTimeInSeconds(), -1);
        BOOST_CHECK_EQUAL (*pos->getPpqPosition(), 3);
        // Isolate plugin latency from the host oversampling filter latency.
        const auto before = node->getLatencySamples();
        proc->setLatencySamples (0);
        node->audioProcessorChanged (proc, AudioProcessorListener::ChangeDetails().withLatencyChanged (true));
        BOOST_CHECK_EQUAL (before - node->getLatencySamples(), (257 + factor - 1) / factor);
        proc->setLatencySamples (257);
        node->audioProcessorChanged (proc, AudioProcessorListener::ChangeDetails().withLatencyChanged (true));
    }
    node->setOversamplingFactor (4);
    for (const int64_t sample : { int64_t (0), int64_t (48000), std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min() }) {
        head.samples = sample;
        const auto position = proc->getPlayHead()->getPosition();
        BOOST_REQUIRE (position.hasValue());
        BOOST_REQUIRE (position->getTimeInSamples().hasValue());
        BOOST_CHECK_EQUAL (*position->getTimeInSamples(), std::clamp (sample, std::numeric_limits<int64_t>::min() / 4, std::numeric_limits<int64_t>::max() / 4) * 4);
    }
    head.samples = nullopt;
    BOOST_CHECK (! proc->getPlayHead()->getPosition()->getTimeInSamples().hasValue());
    f.graph.setPlayHead (nullptr);
    BOOST_CHECK (proc->getPlayHead() == nullptr);
}
BOOST_AUTO_TEST_SUITE_END()
