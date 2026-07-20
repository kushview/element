// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

// Validates the engine's unified multicore render path: one merged schedule
// spanning every warm root graph, driven by the shared worker pool. Each test
// builds identical graph sets in two engines -- one rendering the classic
// serial per-graph loop, one the unified schedule -- and requires matching
// output.
//
// Invariant #0: every non-suspended graph is fully processed every block in
// ALL render modes; only the summing stage is mode-gated. This is what makes
// graph switching seamless, and is asserted explicitly below.

#include <boost/test/unit_test.hpp>

#include <thread>

#include <element/audioengine.hpp>
#include <element/context.hpp>

#include "engine/ionode.hpp"
#include "engine/rootgraph.hpp"
#include "fixture/SignalNodes.h"
#include "testutil.hpp"

using namespace element;
using element::test::fillDeterministic;
using element::test::GainNode;
using element::test::GenNode;

namespace {

constexpr int kBlockSize = 512;
constexpr double kSampleRate = 44100.0;

/** gen -> gain -> audioOut, voiced differently per graph index. The GenNode is
    returned so tests can watch its render counter. */
GenNode* buildSynthGraph (RootGraph& g, int index)
{
    auto* gen = new GenNode (2, 0.2f + 0.15f * (float) index);
    auto* gain = new GainNode (2, 0.5f + 0.1f * (float) index);
    auto* out = new IONode (IONode::audioOutputNode);
    g.addNode (gen);
    g.addNode (gain);
    g.addNode (out);
    for (int ch = 0; ch < 2; ++ch)
    {
        g.connectChannels (PortType::Audio, gen->nodeId, ch, gain->nodeId, ch);
        g.connectChannels (PortType::Audio, gain->nodeId, ch, out->nodeId, ch);
    }
    return gen;
}

/** Two engines with identical graph sets: `serial` renders the classic loop,
    `unified` the merged multicore schedule. */
struct EngineDuo
{
    AudioEnginePtr serial, unified;
    ReferenceCountedArray<RootGraph> serialGraphs, unifiedGraphs;
    Array<GenNode*> serialGens, unifiedGens;

    explicit EngineDuo (int numGraphs)
    {
        auto& ctx = *element::test::context();
        serial = new AudioEngine (ctx, RunMode::Standalone);
        unified = new AudioEngine (ctx, RunMode::Standalone);
        serial->prepareExternalPlayback (kSampleRate, kBlockSize, 2, 2);
        unified->prepareExternalPlayback (kSampleRate, kBlockSize, 2, 2);

        for (int i = 0; i < numGraphs; ++i)
        {
            auto* gs = serialGraphs.add (new RootGraph (ctx));
            serialGens.add (buildSynthGraph (*gs, i));
            serial->addGraph (gs);

            auto* gu = unifiedGraphs.add (new RootGraph (ctx));
            unifiedGens.add (buildSynthGraph (*gu, i));
            unified->addGraph (gu);
        }

        unified->setMulticore (true);
    }

    ~EngineDuo()
    {
        for (auto* g : serialGraphs)
            serial->removeGraph (g);
        for (auto* g : unifiedGraphs)
            unified->removeGraph (g);
        serial->releaseExternalResources();
        unified->releaseExternalResources();
        serial = nullptr;
        unified = nullptr;
        serialGraphs.clear();
        unifiedGraphs.clear();
    }

    void setRenderMode (RootGraph::RenderMode mode)
    {
        for (auto* g : serialGraphs)
            g->setRenderMode (mode);
        for (auto* g : unifiedGraphs)
            g->setRenderMode (mode);
    }

    /** Renders one block through both engines and returns the worst per-sample
        difference between them. */
    double renderBlockAndCompare()
    {
        juce::AudioSampleBuffer input (2, kBlockSize);
        fillDeterministic (input);

        juce::AudioSampleBuffer aOut (input), bOut (input);
        juce::MidiBuffer ma, mb;
        serial->processExternalBuffers (aOut, ma);
        unified->processExternalBuffers (bOut, mb);

        double worst = 0.0;
        for (int ch = 0; ch < aOut.getNumChannels(); ++ch)
            for (int i = 0; i < kBlockSize; ++i)
                worst = jmax (worst, std::abs ((double) aOut.getSample (ch, i) - (double) bOut.getSample (ch, i)));
        return worst;
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE (EngineMulticoreTests)

BOOST_AUTO_TEST_CASE (ParallelModeParity)
{
    // All graphs heard at once (Parallel render mode): unified output must
    // match the serial loop exactly across many blocks.
    EngineDuo duo (4);
    duo.setRenderMode (RootGraph::Parallel);

    double worst = 0.0;
    for (int blk = 0; blk < 100; ++blk)
        worst = jmax (worst, duo.renderBlockAndCompare());

    BOOST_TEST_MESSAGE ("ParallelModeParity worst=" << worst);
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (SingleModeParityAndInvariantZero)
{
    // Single mode: only the current graph is heard, but every warm graph must
    // still be fully processed each block (Invariant #0) so switching stays
    // seamless.
    EngineDuo duo (3);
    duo.setRenderMode (RootGraph::SingleGraph);
    duo.serial->setActiveGraph (0);
    duo.unified->setActiveGraph (0);

    constexpr int kBlocks = 50;
    const int unheardBefore = duo.unifiedGens[1]->renderCount.load();

    double worst = 0.0;
    for (int blk = 0; blk < kBlocks; ++blk)
        worst = jmax (worst, duo.renderBlockAndCompare());

    BOOST_TEST_MESSAGE ("SingleModeParity worst=" << worst);
    BOOST_REQUIRE_EQUAL (worst, 0.0);

    // Invariant #0: the unheard graphs kept rendering, in both engines alike.
    BOOST_REQUIRE_EQUAL (duo.unifiedGens[1]->renderCount.load(), unheardBefore + kBlocks);
    BOOST_REQUIRE_EQUAL (duo.unifiedGens[1]->renderCount.load(),
                         duo.serialGens[1]->renderCount.load());
    BOOST_REQUIRE_EQUAL (duo.unifiedGens[2]->renderCount.load(),
                         duo.serialGens[2]->renderCount.load());
}

BOOST_AUTO_TEST_CASE (GraphSwitchCrossfadeParity)
{
    // Switching the current graph mid-stream must produce identical crossfade
    // ramps in both engines -- including the switch block itself.
    EngineDuo duo (3);
    duo.setRenderMode (RootGraph::SingleGraph);
    duo.serial->setActiveGraph (0);
    duo.unified->setActiveGraph (0);

    double worst = 0.0;
    for (int blk = 0; blk < 30; ++blk)
    {
        if (blk == 10)
        {
            duo.serial->setActiveGraph (2);
            duo.unified->setActiveGraph (2);
        }
        else if (blk == 20)
        {
            duo.serial->setActiveGraph (1);
            duo.unified->setActiveGraph (1);
        }
        worst = jmax (worst, duo.renderBlockAndCompare());
    }

    BOOST_TEST_MESSAGE ("GraphSwitchCrossfadeParity worst=" << worst);
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (SuspendedGraphExcluded)
{
    // A suspended graph leaves the warm set: both engines pass its input
    // through (bypass) and outputs must still match.
    EngineDuo duo (3);
    duo.setRenderMode (RootGraph::Parallel);

    double worst = 0.0;
    for (int blk = 0; blk < 10; ++blk)
        worst = jmax (worst, duo.renderBlockAndCompare());
    BOOST_REQUIRE_EQUAL (worst, 0.0);

    duo.serialGraphs[1]->suspendProcessing (true);
    duo.unifiedGraphs[1]->suspendProcessing (true);
    // Force a synchronous merged rebuild so the warm-set change lands now
    // (in the app this happens via the async invalidation path).
    duo.unified->setMulticore (true);

    for (int blk = 0; blk < 10; ++blk)
        worst = jmax (worst, duo.renderBlockAndCompare());
    BOOST_TEST_MESSAGE ("SuspendedGraphExcluded worst=" << worst);
    BOOST_REQUIRE_EQUAL (worst, 0.0);

    duo.serialGraphs[1]->suspendProcessing (false);
    duo.unifiedGraphs[1]->suspendProcessing (false);
    duo.unified->setMulticore (true);

    for (int blk = 0; blk < 10; ++blk)
        worst = jmax (worst, duo.renderBlockAndCompare());
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (WarmSetChangeNoCrash)
{
    // Adding and removing graphs between blocks must never crash the unified
    // path: a stale merged schedule stays safe to render (graphs are pinned by
    // reference) until the rebuild lands.
    auto& ctx = *element::test::context();
    EngineDuo duo (2);
    duo.setRenderMode (RootGraph::Parallel);

    for (int blk = 0; blk < 5; ++blk)
        duo.renderBlockAndCompare();

    // Add a third graph to both engines mid-stream.
    auto* gs = duo.serialGraphs.add (new RootGraph (ctx));
    duo.serialGens.add (buildSynthGraph (*gs, 2));
    duo.serial->addGraph (gs);

    auto* gu = duo.unifiedGraphs.add (new RootGraph (ctx));
    duo.unifiedGens.add (buildSynthGraph (*gu, 2));
    duo.unified->addGraph (gu);

    // Before the rebuild lands the new graph is silent in the unified engine
    // (by design); these blocks must simply not crash or race.
    duo.renderBlockAndCompare();

    // Land the rebuild synchronously, then require full parity again.
    duo.unified->setMulticore (true);
    double worst = 0.0;
    for (int blk = 0; blk < 10; ++blk)
        worst = jmax (worst, duo.renderBlockAndCompare());
    BOOST_TEST_MESSAGE ("WarmSetChange worst=" << worst);
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (ConcurrentSubmitters)
{
    // Two engine instances submitting jobs to the shared pool from two threads
    // at once -- as a plugin host does with two Element instances whose
    // processBlock calls run concurrently. Each unified engine must match its
    // own serial reference on every block.
    EngineDuo a (3), b (4);
    a.setRenderMode (RootGraph::Parallel);
    b.setRenderMode (RootGraph::Parallel);

    constexpr int kBlocks = 300;
    double worstA = 0.0, worstB = 0.0;

    std::thread ta ([&] {
        for (int blk = 0; blk < kBlocks; ++blk)
            worstA = jmax (worstA, a.renderBlockAndCompare());
    });
    std::thread tb ([&] {
        for (int blk = 0; blk < kBlocks; ++blk)
            worstB = jmax (worstB, b.renderBlockAndCompare());
    });
    ta.join();
    tb.join();

    BOOST_TEST_MESSAGE ("ConcurrentSubmitters worstA=" << worstA << " worstB=" << worstB);
    BOOST_REQUIRE_EQUAL (worstA, 0.0);
    BOOST_REQUIRE_EQUAL (worstB, 0.0);
}

BOOST_AUTO_TEST_CASE (MulticoreToggleRuntime)
{
    // Toggling the engine-level multicore setting between blocks keeps output
    // identical to the always-serial reference.
    EngineDuo duo (3);
    duo.setRenderMode (RootGraph::Parallel);

    double worst = 0.0;
    for (int blk = 0; blk < 30; ++blk)
    {
        if (blk % 7 == 0)
            duo.unified->setMulticore ((blk / 7) % 2 == 0);
        worst = jmax (worst, duo.renderBlockAndCompare());
    }
    BOOST_TEST_MESSAGE ("MulticoreToggleRuntime worst=" << worst);
    BOOST_REQUIRE_EQUAL (worst, 0.0);
}

BOOST_AUTO_TEST_CASE (CpuUsageReported)
{
    // The engine reports its own realtime render load (measured at the graph
    // boundary), independent of any audio device. Kept to sanity bounds -- not
    // a timing-ratio assertion -- so it can't flake under CI load.
    EngineDuo duo (3);
    duo.setRenderMode (RootGraph::Parallel);

    // Nothing rendered yet: no load accumulated.
    BOOST_CHECK_EQUAL (duo.serial->getCpuUsage(), 0.0);

    for (int blk = 0; blk < 50; ++blk)
        duo.renderBlockAndCompare();

    const double serialCpu = duo.serial->getCpuUsage();
    const double unifiedCpu = duo.unified->getCpuUsage();
    BOOST_TEST_MESSAGE ("CpuUsage serial=" << serialCpu << " unified=" << unifiedCpu);

    // A real graph rendered: both engines report a positive, finite fraction.
    BOOST_CHECK_GT (serialCpu, 0.0);
    BOOST_CHECK_GT (unifiedCpu, 0.0);
    BOOST_CHECK_LT (serialCpu, 10.0);
    BOOST_CHECK_LT (unifiedCpu, 10.0);
}

BOOST_AUTO_TEST_SUITE_END()
