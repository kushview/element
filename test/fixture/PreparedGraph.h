#pragma once

#include <element/context.hpp>
#include "engine/graphnode.hpp"
#include "testutil.hpp"

namespace element {

struct PreparedGraph {
    GraphNode graph;

    explicit PreparedGraph (double sampleRate = 44100.0, int blockSize = 512)
        : graph (*element::test::context())
    {
        graph.prepareToRender (sampleRate, blockSize);
    }

    ~PreparedGraph()
    {
        graph.releaseResources();
        graph.clear();
    }
};

} // namespace element
