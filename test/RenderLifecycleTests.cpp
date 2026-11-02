// SPDX-FileCopyrightText: 2026 Kushview, LLC
// Author: jiri-god <318975419+jiri-god@users.noreply.github.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/ionode.hpp"
#include "nodes/audioprocessor.hpp"
#include "nodes/eqfilter.hpp"
#include <thread>
#include <chrono>
using namespace element;
using namespace juce;
namespace {
struct Probe : EQFilterProcessor {
    std::atomic<bool> inside { false }, exit { false }, releasedInside { false }, preparedInside { false };
    bool stall = false;
    void prepareToPlay (double rate, int block) override
    {
        if (inside.load())
            preparedInside = true;
        EQFilterProcessor::prepareToPlay (rate, block);
        setLatencySamples (257);
    }
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override
    {
        inside = true;
        if (stall)
            while (! exit.load())
                std::this_thread::yield();
        else
            std::this_thread::yield();
        inside = false;
    }
    void releaseResources() override
    {
        if (inside.load())
            releasedInside = true;
    }
};
} // namespace
BOOST_AUTO_TEST_SUITE (RenderLifecycleTests)
BOOST_AUTO_TEST_CASE (DisableWaitsForWholeInFlightBlock)
{
    PreparedGraph f (48000, 128);
    auto* proc = new Probe;
    proc->stall = true;
    auto* node = new AudioProcessorNode (proc);
    f.graph.addNode (node);
    f.graph.rebuild();
    std::thread audio ([&] {
        AudioBuffer<float> b (2, 128);
        b.clear();
        MidiBuffer m;
        RenderContext rc (b, b, m, 128);
        f.graph.render (rc);
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (2);
    while (! proc->inside.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    const bool entered = proc->inside.load();
    std::thread finish ([&] {
        std::this_thread::sleep_for (std::chrono::milliseconds (30));
        proc->exit = true;
    });
    node->setEnabled (false);
    finish.join();
    audio.join();
    BOOST_REQUIRE (entered);
    BOOST_CHECK (! proc->releasedInside.load());
    BOOST_CHECK (! proc->preparedInside.load());
    BOOST_CHECK (! node->isEnabled());
    node->setEnabled (true);
    BOOST_CHECK (node->isEnabled());
}

BOOST_AUTO_TEST_CASE (RepeatedReconfigurationWhileRendering)
{
    PreparedGraph f (48000, 128);
    auto* proc = new Probe;
    auto* node = new AudioProcessorNode (proc);
    f.graph.addNode (node);
    f.graph.rebuild();
    std::atomic<bool> done { false };
    std::atomic<int> blocks { 0 };
    std::thread audio ([&] {
        AudioBuffer<float> b (2, 128);
        b.clear();
        MidiBuffer m;
        RenderContext rc (b, b, m, 128);
        while (! done.load()) {
            f.graph.render (rc);
            ++blocks;
        }
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (2);
    while (blocks.load() < 10 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    for (int i = 0; i < 300; ++i) {
        node->setEnabled (false);
        node->setEnabled (true);
    }
    for (int factor : { 2, 4, 1, 2, 1 })
        node->setOversamplingFactor (factor);
    done = true;
    audio.join();
    BOOST_CHECK (! proc->releasedInside.load());
    BOOST_CHECK (! proc->preparedInside.load());
    BOOST_CHECK (node->isEnabled());
    BOOST_CHECK_GT (blocks.load(), 10);
}
BOOST_AUTO_TEST_CASE (QueuedEnableRequestsKeepLatestDesiredState)
{
    PreparedGraph f;
    auto* node = new AudioProcessorNode (new Probe);
    f.graph.addNode (node);
    std::thread control ([&] {
        node->setEnabled (false);
        node->setEnabled (true);
    });
    control.join();
    MessageManager::getInstance()->runDispatchLoopUntil (20);
    BOOST_CHECK (node->isEnabled());
    std::thread os ([&] { node->setOversamplingFactor (4); });
    os.join();
    BOOST_CHECK_EQUAL (node->getOversamplingFactor(), 1);
    MessageManager::getInstance()->runDispatchLoopUntil (20);
    BOOST_CHECK_EQUAL (node->getOversamplingFactor(), 4);
    BOOST_CHECK (node->isEnabled());
}
BOOST_AUTO_TEST_CASE (LatestRequestsOverrideQueuedReconfiguration)
{
    PreparedGraph f;
    auto* node = new AudioProcessorNode (new Probe);
    f.graph.addNode (node);
    std::thread control ([&] {
        node->setOversamplingFactor (2);
        node->setEnabled (false);
    });
    control.join();
    node->setOversamplingFactor (4);
    MessageManager::getInstance()->runDispatchLoopUntil (20);
    BOOST_CHECK_EQUAL (node->getOversamplingFactor(), 4);
    BOOST_CHECK (! node->isEnabled());

    std::thread control2 ([&] {
        node->setEnabled (true);
        node->setOversamplingFactor (1);
    });
    control2.join();
    MessageManager::getInstance()->runDispatchLoopUntil (20);
    BOOST_CHECK_EQUAL (node->getOversamplingFactor(), 1);
    BOOST_CHECK (node->isEnabled());
}
BOOST_AUTO_TEST_SUITE_END()
