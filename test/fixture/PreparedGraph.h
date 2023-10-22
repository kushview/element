#pragma once

#include <element/context.hpp>
#include "engine/graphnode.hpp"

namespace element {

struct PreparedGraph {
    Context context;
    GraphNode graph;

    explicit PreparedGraph (double sampleRate = 44100.0, int blockSize = 512)
        : context (RunMode::Standalone),
          graph (context)
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
