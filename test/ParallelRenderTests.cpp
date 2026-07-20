// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

// Validates that the parallel render path (GraphNode::setMulticore) with
// its non-reused, always-copy buffer model produces the same output as the
// original sequential path across a range of routing topologies.

#include <boost/test/unit_test.hpp>

#include <functional>

#include <element/context.hpp>

#include "fixture/PreparedGraph.h"
#include "fixture/SignalNodes.h"
#include "engine/graphnode.hpp"
#include "engine/ionode.hpp"

using namespace element;
using element::test::fillDeterministic;
using element::test::GainNode;
using element::test::GenNode;

namespace {

constexpr int kBlockSize = 512;

struct CompareResult {
    int channels = 0;
    double maxAbsDiff = 0.0;
    bool exact = true;
};

/** Builds the same graph twice (sequential + parallel), renders both from
    identical input, and reports the per-sample difference of the graph output. */
CompareResult runAndCompare (const std::function<void (GraphNode&)>& build)
{
    juce::AudioSampleBuffer input (2, kBlockSize);
    fillDeterministic (input);

    PreparedGraph seq (44100.0, kBlockSize);
    build (seq.graph);
    seq.graph.rebuild();

    PreparedGraph par (44100.0, kBlockSize);
    build (par.graph);
    par.graph.setMulticore (true);
    par.graph.rebuild();

    BOOST_REQUIRE (! seq.graph.isMulticore());
    BOOST_REQUIRE (par.graph.isMulticore());

    juce::AudioSampleBuffer aOut (input), bOut (input);
    juce::MidiBuffer ma, mb;
    RenderContext rcA (aOut, aOut, ma, kBlockSize);
    RenderContext rcB (bOut, bOut, mb, kBlockSize);
    seq.graph.render (rcA);
    par.graph.render (rcB);

    CompareResult r;
    r.channels = jmin (aOut.getNumChannels(), bOut.getNumChannels());
    BOOST_REQUIRE_EQUAL (aOut.getNumChannels(), bOut.getNumChannels());
    for (int ch = 0; ch < r.channels; ++ch)
        for (int i = 0; i < kBlockSize; ++i) {
            const double d = std::abs ((double) aOut.getSample (ch, i) - (double) bOut.getSample (ch, i));
            r.maxAbsDiff = jmax (r.maxAbsDiff, d);
            if (d != 0.0)
                r.exact = false;
        }
    return r;
}

} // namespace

BOOST_AUTO_TEST_SUITE (ParallelRenderTests)

BOOST_AUTO_TEST_CASE (LinearChain)
{
    // audioIn -> gain -> gain -> audioOut
    auto r = runAndCompare ([] (GraphNode& g) {
        auto* in = new IONode (IONode::audioInputNode);
        auto* g1 = new GainNode (2, 0.7f);
        auto* g2 = new GainNode (2, 0.5f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (in);
        g.addNode (g1);
        g.addNode (g2);
        g.addNode (out);
        for (int ch = 0; ch < 2; ++ch) {
            g.connectChannels (PortType::Audio, in->nodeId, ch, g1->nodeId, ch);
            g.connectChannels (PortType::Audio, g1->nodeId, ch, g2->nodeId, ch);
            g.connectChannels (PortType::Audio, g2->nodeId, ch, out->nodeId, ch);
        }
    });
    BOOST_TEST_MESSAGE ("LinearChain maxAbsDiff=" << r.maxAbsDiff);
    BOOST_REQUIRE (r.exact);
}

BOOST_AUTO_TEST_CASE (FanOut)
{
    // audioIn -> {gainA, gainB}; both -> audioOut (ch0, ch1)
    auto r = runAndCompare ([] (GraphNode& g) {
        auto* in = new IONode (IONode::audioInputNode);
        auto* a = new GainNode (1, 0.8f);
        auto* b = new GainNode (1, 0.3f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (in);
        g.addNode (a);
        g.addNode (b);
        g.addNode (out);
        // one source fanned out to two independent consumers
        g.connectChannels (PortType::Audio, in->nodeId, 0, a->nodeId, 0);
        g.connectChannels (PortType::Audio, in->nodeId, 0, b->nodeId, 0);
        g.connectChannels (PortType::Audio, a->nodeId, 0, out->nodeId, 0);
        g.connectChannels (PortType::Audio, b->nodeId, 0, out->nodeId, 1);
    });
    BOOST_TEST_MESSAGE ("FanOut maxAbsDiff=" << r.maxAbsDiff);
    BOOST_REQUIRE (r.exact);
}

BOOST_AUTO_TEST_CASE (MultiInputMixing)
{
    // Three dedicated generators summed into one gain -> audioOut.
    // Sources are dedicated (no fan-out), so summation order matches: exact.
    auto r = runAndCompare ([] (GraphNode& g) {
        auto* g1 = new GenNode (1, 0.5f);
        auto* g2 = new GenNode (1, 0.7f);
        auto* g3 = new GenNode (1, 0.9f);
        auto* mix = new GainNode (1, 1.0f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (g1);
        g.addNode (g2);
        g.addNode (g3);
        g.addNode (mix);
        g.addNode (out);
        g.connectChannels (PortType::Audio, g1->nodeId, 0, mix->nodeId, 0);
        g.connectChannels (PortType::Audio, g2->nodeId, 0, mix->nodeId, 0);
        g.connectChannels (PortType::Audio, g3->nodeId, 0, mix->nodeId, 0);
        g.connectChannels (PortType::Audio, mix->nodeId, 0, out->nodeId, 0);
    });
    BOOST_TEST_MESSAGE ("MultiInputMixing maxAbsDiff=" << r.maxAbsDiff);
    BOOST_REQUIRE (r.exact);
}

BOOST_AUTO_TEST_CASE (FanOutThenMix)
{
    // gen -> {passA, passB}; passA + gen -> mix. gen has fan-out AND feeds the mix,
    // so the sequential builder may accumulate in a different order: allow ULP diff.
    auto r = runAndCompare ([] (GraphNode& g) {
        auto* gen = new GenNode (1, 0.6f);
        auto* a = new GainNode (1, 0.9f);
        auto* mix = new GainNode (1, 1.0f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (gen);
        g.addNode (a);
        g.addNode (mix);
        g.addNode (out);
        g.connectChannels (PortType::Audio, gen->nodeId, 0, a->nodeId, 0);
        g.connectChannels (PortType::Audio, a->nodeId, 0, mix->nodeId, 0);
        g.connectChannels (PortType::Audio, gen->nodeId, 0, mix->nodeId, 0);
        g.connectChannels (PortType::Audio, mix->nodeId, 0, out->nodeId, 0);
    });
    BOOST_TEST_MESSAGE ("FanOutThenMix maxAbsDiff=" << r.maxAbsDiff);
    BOOST_REQUIRE_LT (r.maxAbsDiff, 1.0e-6);
}

BOOST_AUTO_TEST_CASE (StereoCrossover)
{
    // Stereo generator with swapped channel routing to output.
    auto r = runAndCompare ([] (GraphNode& g) {
        auto* gen = new GenNode (2, 1.0f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (gen);
        g.addNode (out);
        g.connectChannels (PortType::Audio, gen->nodeId, 0, out->nodeId, 1);
        g.connectChannels (PortType::Audio, gen->nodeId, 1, out->nodeId, 0);
    });
    BOOST_TEST_MESSAGE ("StereoCrossover maxAbsDiff=" << r.maxAbsDiff);
    BOOST_REQUIRE (r.exact);
}

BOOST_AUTO_TEST_CASE (IndependentChains)
{
    // Two fully independent chains that a scheduler could run concurrently.
    auto r = runAndCompare ([] (GraphNode& g) {
        auto* in = new IONode (IONode::audioInputNode);
        auto* la = new GainNode (1, 0.4f);
        auto* lb = new GainNode (1, 0.6f);
        auto* ra = new GainNode (1, 0.8f);
        auto* rb = new GainNode (1, 0.5f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (in);
        g.addNode (la);
        g.addNode (lb);
        g.addNode (ra);
        g.addNode (rb);
        g.addNode (out);
        // chain 1 on ch0, chain 2 on ch1
        g.connectChannels (PortType::Audio, in->nodeId, 0, la->nodeId, 0);
        g.connectChannels (PortType::Audio, la->nodeId, 0, lb->nodeId, 0);
        g.connectChannels (PortType::Audio, lb->nodeId, 0, out->nodeId, 0);
        g.connectChannels (PortType::Audio, in->nodeId, 1, ra->nodeId, 0);
        g.connectChannels (PortType::Audio, ra->nodeId, 0, rb->nodeId, 0);
        g.connectChannels (PortType::Audio, rb->nodeId, 0, out->nodeId, 1);
    });
    BOOST_TEST_MESSAGE ("IndependentChains maxAbsDiff=" << r.maxAbsDiff);
    BOOST_REQUIRE (r.exact);

    // Sanity: the parallel schedule actually split into multiple tasks.
    PreparedGraph par (44100.0, kBlockSize);
    auto* in = new IONode (IONode::audioInputNode);
    auto* la = new GainNode (1, 0.4f);
    auto* out = new IONode (IONode::audioOutputNode);
    par.graph.addNode (in);
    par.graph.addNode (la);
    par.graph.addNode (out);
    par.graph.connectChannels (PortType::Audio, in->nodeId, 0, la->nodeId, 0);
    par.graph.connectChannels (PortType::Audio, la->nodeId, 0, out->nodeId, 0);
    par.graph.setMulticore (true);
    par.graph.rebuild();
    BOOST_REQUIRE_GT (par.graph.getNumRenderTasks(), 0);
}

BOOST_AUTO_TEST_CASE (WideParallelStress)
{
    // Many independent generator+gain chains summed into one output. This gives
    // the pool lots of concurrently-runnable tasks; render many blocks and require
    // the parallel output to match the sequential reference on every block.
    constexpr int kChains = 24;
    constexpr int kBlocks = 200;

    auto build = [] (GraphNode& g) {
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (out);
        auto* mix = new GainNode (1, 1.0f);
        g.addNode (mix);
        for (int c = 0; c < kChains; ++c) {
            auto* gen = new GenNode (1, 0.05f + 0.01f * (float) c);
            auto* gain = new GainNode (1, 0.5f);
            g.addNode (gen);
            g.addNode (gain);
            g.connectChannels (PortType::Audio, gen->nodeId, 0, gain->nodeId, 0);
            g.connectChannels (PortType::Audio, gain->nodeId, 0, mix->nodeId, 0);
        }
        g.connectChannels (PortType::Audio, mix->nodeId, 0, out->nodeId, 0);
    };

    PreparedGraph seq (44100.0, kBlockSize);
    build (seq.graph);
    seq.graph.rebuild();

    PreparedGraph par (44100.0, kBlockSize);
    build (par.graph);
    par.graph.setMulticore (true);
    par.graph.rebuild();
    BOOST_REQUIRE_GT (par.graph.getNumRenderTasks(), 2);

    juce::AudioSampleBuffer input (2, kBlockSize);
    fillDeterministic (input);

    double worst = 0.0;
    for (int blk = 0; blk < kBlocks; ++blk) {
        juce::AudioSampleBuffer aOut (input), bOut (input);
        juce::MidiBuffer ma, mb;
        RenderContext rcA (aOut, aOut, ma, kBlockSize);
        RenderContext rcB (bOut, bOut, mb, kBlockSize);
        seq.graph.render (rcA);
        par.graph.render (rcB);
        for (int ch = 0; ch < aOut.getNumChannels(); ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                worst = jmax (worst, std::abs ((double) aOut.getSample (ch, i) - (double) bOut.getSample (ch, i)));
    }

    BOOST_TEST_MESSAGE ("WideParallelStress worst=" << worst << " tasks=" << par.graph.getNumRenderTasks());
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (DisengagedWorkgroupTolerated)
{
    // A disengaged (default) AudioWorkgroup must be tolerated: workers run as
    // plain realtime threads and output stays correct.
    auto build = [] (GraphNode& g) {
        auto* in = new IONode (IONode::audioInputNode);
        auto* g1 = new GainNode (2, 0.7f);
        auto* g2 = new GainNode (2, 0.5f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (in);
        g.addNode (g1);
        g.addNode (g2);
        g.addNode (out);
        for (int ch = 0; ch < 2; ++ch) {
            g.connectChannels (PortType::Audio, in->nodeId, ch, g1->nodeId, ch);
            g.connectChannels (PortType::Audio, g1->nodeId, ch, g2->nodeId, ch);
            g.connectChannels (PortType::Audio, g2->nodeId, ch, out->nodeId, ch);
        }
    };

    juce::AudioSampleBuffer input (2, kBlockSize);
    fillDeterministic (input);

    PreparedGraph seq (44100.0, kBlockSize);
    build (seq.graph);
    seq.graph.rebuild();

    PreparedGraph par (44100.0, kBlockSize);
    build (par.graph);
    par.graph.setMulticore (true);
    par.graph.setAudioWorkgroup (juce::AudioWorkgroup {}); // disengaged
    par.graph.rebuild();

    double worst = 0.0;
    for (int blk = 0; blk < 8; ++blk) {
        juce::AudioSampleBuffer aOut (input), bOut (input);
        juce::MidiBuffer ma, mb;
        RenderContext rcA (aOut, aOut, ma, kBlockSize);
        RenderContext rcB (bOut, bOut, mb, kBlockSize);
        seq.graph.render (rcA);
        par.graph.render (rcB);
        for (int ch = 0; ch < aOut.getNumChannels(); ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                worst = jmax (worst, std::abs ((double) aOut.getSample (ch, i) - (double) bOut.getSample (ch, i)));
    }
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (RuntimeToggle)
{
    // Toggling parallel rendering on and off between blocks (as the engine does
    // when the setting changes) must keep producing the sequential-equivalent
    // output and must not crash on the schedule swap.
    auto build = [] (GraphNode& g) {
        auto* in = new IONode (IONode::audioInputNode);
        auto* a = new GainNode (2, 0.6f);
        auto* b = new GainNode (2, 0.4f);
        auto* out = new IONode (IONode::audioOutputNode);
        g.addNode (in);
        g.addNode (a);
        g.addNode (b);
        g.addNode (out);
        for (int ch = 0; ch < 2; ++ch) {
            g.connectChannels (PortType::Audio, in->nodeId, ch, a->nodeId, ch);
            g.connectChannels (PortType::Audio, a->nodeId, ch, b->nodeId, ch);
            g.connectChannels (PortType::Audio, b->nodeId, ch, out->nodeId, ch);
        }
    };

    juce::AudioSampleBuffer input (2, kBlockSize);
    fillDeterministic (input);

    PreparedGraph ref (44100.0, kBlockSize);
    build (ref.graph);
    ref.graph.rebuild();

    PreparedGraph tog (44100.0, kBlockSize);
    build (tog.graph);
    tog.graph.rebuild();

    double worst = 0.0;
    for (int blk = 0; blk < 40; ++blk) {
        // Flip parallel state every few blocks.
        tog.graph.setMulticore ((blk / 3) % 2 == 1);

        juce::AudioSampleBuffer aOut (input), bOut (input);
        juce::MidiBuffer ma, mb;
        RenderContext rcA (aOut, aOut, ma, kBlockSize);
        RenderContext rcB (bOut, bOut, mb, kBlockSize);
        ref.graph.render (rcA);
        tog.graph.render (rcB);
        for (int ch = 0; ch < aOut.getNumChannels(); ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                worst = jmax (worst, std::abs ((double) aOut.getSample (ch, i) - (double) bOut.getSample (ch, i)));
    }
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (MultiOutputIONoRace)
{
    // Two audio output IO nodes both accumulate into the graph's shared
    // currentAudioOutputBuffer. audioThreadOnly must serialize them on the audio
    // thread so there is no data race (verified under TSan) and the result matches
    // the sequential path (their sum order may differ, so allow ULP-scale diff).
    constexpr int kBlocks = 64;

    auto build = [] (GraphNode& g) {
        auto* out1 = new IONode (IONode::audioOutputNode);
        auto* out2 = new IONode (IONode::audioOutputNode);
        auto* gen1 = new GenNode (1, 0.5f);
        auto* gen2 = new GenNode (1, 0.35f);
        g.addNode (out1);
        g.addNode (out2);
        g.addNode (gen1);
        g.addNode (gen2);
        g.connectChannels (PortType::Audio, gen1->nodeId, 0, out1->nodeId, 0);
        g.connectChannels (PortType::Audio, gen2->nodeId, 0, out2->nodeId, 0);
    };

    juce::AudioSampleBuffer input (2, kBlockSize);
    fillDeterministic (input);

    PreparedGraph seq (44100.0, kBlockSize);
    build (seq.graph);
    seq.graph.rebuild();

    PreparedGraph par (44100.0, kBlockSize);
    build (par.graph);
    par.graph.setMulticore (true);
    par.graph.rebuild();

    // Both audio output IO nodes must be pinned to the audio thread.
    BOOST_REQUIRE_EQUAL (par.graph.getNumAudioThreadOnlyTasks(), 2);

    double worst = 0.0;
    for (int blk = 0; blk < kBlocks; ++blk) {
        juce::AudioSampleBuffer aOut (input), bOut (input);
        juce::MidiBuffer ma, mb;
        RenderContext rcA (aOut, aOut, ma, kBlockSize);
        RenderContext rcB (bOut, bOut, mb, kBlockSize);
        seq.graph.render (rcA);
        par.graph.render (rcB);
        for (int ch = 0; ch < aOut.getNumChannels(); ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                worst = jmax (worst, std::abs ((double) aOut.getSample (ch, i) - (double) bOut.getSample (ch, i)));
    }
    BOOST_TEST_MESSAGE ("MultiOutputIONoRace worst=" << worst);
    BOOST_REQUIRE_LT (worst, 1.0e-6);
}

BOOST_AUTO_TEST_SUITE_END()
