// SPDX-FileCopyrightText: 2026 Kushview, LLC
// Author: jiri-god <318975419+jiri-god@users.noreply.github.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <boost/test/unit_test.hpp>
#include "fixture/PreparedGraph.h"
#include "fixture/TestNode.h"
#include "engine/ionode.hpp"
using namespace element;
using namespace juce;
BOOST_AUTO_TEST_SUITE (GraphContinuityTests)
BOOST_AUTO_TEST_CASE (UnchangedDelayArcsKeepTheirHistoryAcrossRebuild)
{
    struct Source : TestNode {
        using Processor::setLatencySamples;
        Source (float v, int delay) : TestNode (0, 1, 0, 0), value (v) { setLatencySamples (delay); }
        void render (RenderContext& rc) override
        {
            for (int i = 0; i < rc.audio.getNumSamples(); ++i)
                rc.audio.setSample (0, i, value);
        }
        float value;
    };
    PreparedGraph f (48000, 128);
    auto& g = f.graph;
    auto* a = g.addNode (new Source (.25f, 0));
    auto* b = new Source (.5f, 257);
    g.addNode (b);
    auto* out = g.addNode (new IONode (IONode::audioOutputNode));
    BOOST_REQUIRE (g.connectChannels (PortType::Audio, a->nodeId, 0, out->nodeId, 0));
    BOOST_REQUIRE (g.connectChannels (PortType::Audio, b->nodeId, 0, out->nodeId, 0));
    g.rebuild();
    AudioBuffer<float> audio (2, 128);
    MidiBuffer midi;
    RenderContext rc (audio, audio, midi, 128);
    for (int n = 0; n < 5; ++n)
        g.render (rc);
    BOOST_CHECK_SMALL (audio.getSample (0, 127) - .75f, 1e-6f);
    g.addNode (new Source (0, 0));
    g.rebuild();
    g.render (rc);
    for (int i = 0; i < 128; ++i)
        BOOST_CHECK_SMALL (audio.getSample (0, i) - .75f, 1e-6f);
    // Smaller and varying device blocks must preserve the running history.
    for (int size : { 1, 64, 127, 32, 128 }) {
        AudioBuffer<float> block (2, size);
        RenderContext context (block, block, midi, size);
        g.render (context);
        for (int i = 0; i < size; ++i)
            BOOST_CHECK_SMALL (block.getSample (0, i) - .75f, 1e-6f);
    }

    // A changed delay must start a fresh history, not inherit a stale ring.
    b->setLatencySamples (129);
    g.rebuild();
    g.render (rc);
    for (int i = 0; i < 128; ++i)
        BOOST_CHECK_SMALL (audio.getSample (0, i) - .5f, 1e-6f);
    g.render (rc);
    BOOST_CHECK_SMALL (audio.getSample (0, 127) - .75f, 1e-6f);
}
BOOST_AUTO_TEST_SUITE_END()
