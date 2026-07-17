// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/processor.hpp>

#include "engine/graphnode.hpp"
#include "fixture/TestNode.h"

using namespace element;
using namespace juce;

namespace {

/** A TestNode that produces non-empty state, so saved local MIDI programs are
    retained (getMidiPrograms() filters out empty slots). */
class StateNode : public TestNode {
public:
    StateNode() : TestNode (0, 0, 1, 1) {}

    void getState (MemoryBlock& block) override
    {
        block.reset();
        block.append ("state", 5);
    }
};

/** Returns true if the node has a saved program at the given number. */
bool hasProgram (ProcessorPtr node, int program)
{
    for (const auto& info : node->getMidiPrograms())
        if (info.program == program)
            return true;
    return false;
}

} // namespace

BOOST_AUTO_TEST_SUITE (MidiProgramTests)

BOOST_AUTO_TEST_CASE (SaveToSpecificSlot)
{
    Context context;
    GraphNode graph (context);
    ProcessorPtr node = graph.addNode (new StateNode());

    BOOST_REQUIRE_EQUAL (node->getMidiPrograms().size(), 0);

    // Saving into an arbitrary slot must not change the active program.
    const int active = node->getMidiProgram();
    node->saveMidiProgram (5);
    BOOST_REQUIRE_EQUAL (node->getMidiProgram(), active);
    BOOST_REQUIRE_EQUAL (node->getMidiPrograms().size(), 1);
    BOOST_REQUIRE (hasProgram (node, 5));

    node->saveMidiProgram (0);
    node->saveMidiProgram (2);
    BOOST_REQUIRE_EQUAL (node->getMidiPrograms().size(), 3);

    // Out-of-range saves are ignored.
    node->saveMidiProgram (-1);
    node->saveMidiProgram (128);
    BOOST_REQUIRE_EQUAL (node->getMidiPrograms().size(), 3);

    graph.clear();
}

BOOST_AUTO_TEST_CASE (NextAvailableFromSelection)
{
    Context context;
    GraphNode graph (context);
    ProcessorPtr node = graph.addNode (new StateNode());

    node->saveMidiProgram (0);
    node->saveMidiProgram (1);
    node->saveMidiProgram (2);
    node->saveMidiProgram (5);

    // Search upward from the selected program, skipping occupied slots.
    BOOST_REQUIRE_EQUAL (node->nextAvailableMidiProgram (0), 3);
    BOOST_REQUIRE_EQUAL (node->nextAvailableMidiProgram (2), 3);
    BOOST_REQUIRE_EQUAL (node->nextAvailableMidiProgram (5), 6);

    graph.clear();
}

BOOST_AUTO_TEST_CASE (NextAvailableWrapsAround)
{
    Context context;
    GraphNode graph (context);
    ProcessorPtr node = graph.addNode (new StateNode());

    node->saveMidiProgram (127);
    // From 127 the search wraps past the end back to 0.
    BOOST_REQUIRE_EQUAL (node->nextAvailableMidiProgram (127), 0);

    graph.clear();
}

BOOST_AUTO_TEST_CASE (NextAvailableFullReturnsMinusOne)
{
    Context context;
    GraphNode graph (context);
    ProcessorPtr node = graph.addNode (new StateNode());

    for (int i = 0; i < 128; ++i)
        node->saveMidiProgram (i);
    BOOST_REQUIRE_EQUAL (node->getMidiPrograms().size(), 128);

    BOOST_REQUIRE_EQUAL (node->nextAvailableMidiProgram (0), -1);
    BOOST_REQUIRE_EQUAL (node->nextAvailableMidiProgram (64), -1);

    graph.clear();
}

BOOST_AUTO_TEST_SUITE_END()
