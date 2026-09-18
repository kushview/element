// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <element/context.hpp>
#include <element/node.hpp>
#include <element/session.hpp>

#include "engine/rootgraph.hpp"

using namespace element;

namespace {

juce::StringArray graphNames (const Session& session)
{
    juce::StringArray names;
    for (int i = 0; i < session.getNumGraphs(); ++i)
        names.add (session.getGraph (i).getName());
    return names;
}

} // namespace

BOOST_AUTO_TEST_SUITE (SessionTests)

BOOST_AUTO_TEST_CASE (MoveGraph)
{
    Context context (RunMode::Standalone);
    auto session = context.session();
    BOOST_REQUIRE (session != nullptr);

    for (const auto* name : { "A", "B", "C" })
        session->addGraph (Node::createDefaultGraph (name), juce::String (name) == "B");

    BOOST_REQUIRE_EQUAL (session->getNumGraphs(), 3);
    BOOST_REQUIRE_EQUAL (session->getActiveGraphIndex(), 1);

    // move the active graph up
    BOOST_REQUIRE (session->moveGraph (1, 0));
    BOOST_REQUIRE (graphNames (*session) == juce::StringArray ({ "B", "A", "C" }));
    BOOST_REQUIRE_EQUAL (session->getActiveGraphIndex(), 0);
    BOOST_REQUIRE_EQUAL (session->getActiveGraph().getName().toStdString(), "B");

    // move the active graph to the end
    BOOST_REQUIRE (session->moveGraph (0, 2));
    BOOST_REQUIRE (graphNames (*session) == juce::StringArray ({ "A", "C", "B" }));
    BOOST_REQUIRE_EQUAL (session->getActiveGraphIndex(), 2);

    // moving another graph keeps the active graph active
    BOOST_REQUIRE (session->moveGraph (0, 1));
    BOOST_REQUIRE (graphNames (*session) == juce::StringArray ({ "C", "A", "B" }));
    BOOST_REQUIRE_EQUAL (session->getActiveGraphIndex(), 2);
    BOOST_REQUIRE_EQUAL (session->getActiveGraph().getName().toStdString(), "B");

    // no-ops
    BOOST_REQUIRE (! session->moveGraph (1, 1));
    BOOST_REQUIRE (! session->moveGraph (0, 3));
    BOOST_REQUIRE (! session->moveGraph (-1, 0));
    BOOST_REQUIRE (graphNames (*session) == juce::StringArray ({ "C", "A", "B" }));
    BOOST_REQUIRE_EQUAL (session->getActiveGraphIndex(), 2);
}

BOOST_AUTO_TEST_CASE (MoveEngineGraph)
{
    Context context (RunMode::Standalone);
    auto engine = context.audio();
    BOOST_REQUIRE (engine != nullptr);

    RootGraph a (context), b (context), c (context);
    for (auto* graph : { &a, &b, &c })
        BOOST_REQUIRE (engine->addGraph (graph));

    BOOST_REQUIRE_EQUAL (a.getEngineIndex(), 0);
    BOOST_REQUIRE_EQUAL (b.getEngineIndex(), 1);
    BOOST_REQUIRE_EQUAL (c.getEngineIndex(), 2);

    engine->setActiveGraphIndex (1);
    BOOST_REQUIRE (engine->moveGraph (1, 2));
    BOOST_REQUIRE_EQUAL (a.getEngineIndex(), 0);
    BOOST_REQUIRE_EQUAL (c.getEngineIndex(), 1);
    BOOST_REQUIRE_EQUAL (b.getEngineIndex(), 2);
    BOOST_REQUIRE (engine->getGraph (2) == &b);
    BOOST_REQUIRE_EQUAL (engine->getActiveGraphIndex(), 2);

    BOOST_REQUIRE (engine->moveGraph (2, 0));
    BOOST_REQUIRE_EQUAL (b.getEngineIndex(), 0);
    BOOST_REQUIRE_EQUAL (a.getEngineIndex(), 1);
    BOOST_REQUIRE_EQUAL (c.getEngineIndex(), 2);
    BOOST_REQUIRE (engine->getGraph (0) == &b);
    BOOST_REQUIRE_EQUAL (engine->getActiveGraphIndex(), 0);

    // no-ops
    BOOST_REQUIRE (! engine->moveGraph (0, 3));
    BOOST_REQUIRE (! engine->moveGraph (1, 1));
    BOOST_REQUIRE (! engine->moveGraph (-1, 0));
    BOOST_REQUIRE_EQUAL (b.getEngineIndex(), 0);
    BOOST_REQUIRE_EQUAL (engine->getActiveGraphIndex(), 0);

    // the engine holds raw pointers: detach before the graphs go out of scope
    for (auto* graph : { &a, &b, &c })
        engine->removeGraph (graph);
}

BOOST_AUTO_TEST_SUITE_END()
