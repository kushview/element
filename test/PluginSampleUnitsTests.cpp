// SPDX-FileCopyrightText: 2026 Kushview, LLC
// Author: jiri-god <318975419+jiri-god@users.noreply.github.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/ionode.hpp"
#include "nodes/audioprocessor.hpp"
#include "nodes/eqfilter.hpp"
#include "nodes/scriptnode.hpp"
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
// Same direct Processor path used by CLAP and native nodes, without a plugin wrapper.
struct DirectProbe : TestNode {
    DirectProbe() : TestNode (2, 2, 0, 0) {}
    double renderingRate = 0;
    int renderingBlock = 0;
    void prepareToRender (double rate, int block) override
    {
        renderingRate = rate; renderingBlock = block;
        setLatencySamples (257);
    }
    using Processor::setLatencySamples;
};
struct Head : AudioPlayHead {
    Optional<int64_t> samples { -48000 };
    bool available = true;
    Optional<PositionInfo> getPosition() const override
    {
        if (!available) return nullopt;
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
BOOST_AUTO_TEST_CASE (DirectProcessorsUseLocalSamplesAndParentLatencyUnits)
{
    PreparedGraph f (48000, 128); Head head; f.graph.setPlayHead (&head);
    auto* node = new DirectProbe; f.graph.addNode (node);
    auto* clock = node->getPlayHead(); BOOST_REQUIRE (clock != nullptr);
    for (int factor : { 1, 2, 4, 8, 1 }) {
        node->setOversamplingFactor (factor);
        BOOST_CHECK_EQUAL (node->renderingRate, 48000 * factor);
        BOOST_CHECK_EQUAL (node->renderingBlock, 128 * factor);
        const auto pos = clock->getPosition(); BOOST_REQUIRE (pos.hasValue());
        BOOST_CHECK_EQUAL (*pos->getTimeInSamples(), -48000 * factor);
        BOOST_CHECK_EQUAL (*pos->getTimeInSeconds(), -1);
        BOOST_CHECK_EQUAL (*pos->getPpqPosition(), 3);
        BOOST_CHECK_EQUAL (*pos->getBpm(), 120);
        const auto intrinsicAndFilter = node->getLatencySamples();
        node->setLatencySamples (0);
        const auto filterOnly = node->getLatencySamples();
        BOOST_CHECK_EQUAL (intrinsicAndFilter - filterOnly, (257 + factor - 1) / factor);
        node->setDelayCompensation (1.0);
        BOOST_CHECK_EQUAL (node->getLatencySamples() - filterOnly, 48);
        node->setDelayCompensation (0);
        node->setLatencySamples (257);
    }
    Head replacement; replacement.samples = 123;
    f.graph.setPlayHead (&replacement);
    BOOST_CHECK_EQUAL (*clock->getPosition()->getTimeInSamples(), 123);
    replacement.samples = nullopt;
    BOOST_CHECK (!clock->getPosition()->getTimeInSamples().hasValue());
    replacement.available = false;
    BOOST_CHECK (!clock->getPosition().hasValue());
    f.graph.setPlayHead (nullptr);
    BOOST_CHECK (node->getPlayHead() == nullptr);
    BOOST_CHECK (!clock->getPosition().hasValue());
}

BOOST_AUTO_TEST_CASE (NestedGraphsScaleEachBoundaryExactlyOnce)
{
    PreparedGraph f (48000, 128); Head head; f.graph.setPlayHead (&head);
    auto* nested = new GraphNode (*element::test::context()); f.graph.addNode (nested);
    nested->setOversamplingFactor (2);
    auto* direct = new DirectProbe; nested->addNode (direct);
    auto* plugin = new Probe; auto* wrapped = new AudioProcessorNode (plugin); nested->addNode (wrapped);
    for (int childFactor : {1, 2, 4, 1}) {
        direct->setOversamplingFactor (childFactor); wrapped->setOversamplingFactor (childFactor);
        const int totalFactor = 2 * childFactor;
        BOOST_CHECK_EQUAL (direct->renderingRate, 48000 * totalFactor);
        BOOST_CHECK_EQUAL (plugin->getSampleRate(), 48000 * totalFactor);
        BOOST_CHECK_EQUAL (*direct->getPlayHead()->getPosition()->getTimeInSamples(), -48000 * totalFactor);
        BOOST_CHECK_EQUAL (*plugin->getPlayHead()->getPosition()->getTimeInSamples(), -48000 * totalFactor);
        BOOST_CHECK_EQUAL (*nested->getPlayHead()->getPosition()->getTimeInSamples(), -96000);
        BOOST_CHECK_EQUAL (*head.getPosition()->getTimeInSamples(), -48000);
        BOOST_CHECK_EQUAL (*plugin->getPlayHead()->getPosition()->getTimeInSeconds(), -1);
    }
    nested->setOversamplingFactor (1);
    BOOST_CHECK_EQUAL (*direct->getPlayHead()->getPosition()->getTimeInSamples(), -48000);
    BOOST_CHECK_EQUAL (*plugin->getPlayHead()->getPosition()->getTimeInSamples(), -48000);
    f.graph.setPlayHead (nullptr);
    BOOST_CHECK (nested->getPlayHead() == nullptr);
    BOOST_CHECK (direct->getPlayHead() == nullptr);
    BOOST_CHECK (plugin->getPlayHead() == nullptr);
}
BOOST_AUTO_TEST_CASE (ScriptReceivesLocalClockBeforeAndAfterReloadOrRebind)
{
    PreparedGraph f (48000, 128); Head head; f.graph.setPlayHead (&head);
    const String code = R"(
-- @script clock_probe
-- @type DSP
local function layout() return { audio = { 2, 2 }, midi = { 0, 0 } } end
local function process(a, m, p, c, t)
    a:set(1, 1, t:frame())
    a:set(2, 1, t:seconds())
end
return { type = 'DSP', layout = layout, process = process }
)";
    auto* node = new ScriptNode;
    const auto loaded = node->loadScript (code);
    BOOST_REQUIRE_MESSAGE (loaded.wasOk(), loaded.getErrorMessage().toStdString());
    f.graph.addNode (node);
    AudioBuffer<float> audio (2, 16), cv (1, 16); MidiBuffer midi;
    RenderContext rc (audio, cv, midi, 16);
    for (int factor : {1, 2, 4, 1}) {
        node->setOversamplingFactor (factor);
        for (int step = 0; step < 3; ++step) {
            if (step == 1) BOOST_REQUIRE (node->loadScript (code).wasOk());
            if (step == 2) f.graph.setPlayHead (&head);
            audio.clear(); node->render (rc);
            BOOST_CHECK_EQUAL (audio.getSample (0, 0), -48000 * factor);
            BOOST_CHECK_EQUAL (audio.getSample (1, 0), -1);
        }
    }
}
BOOST_AUTO_TEST_SUITE_END()
