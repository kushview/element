#pragma once
#include "engine/graphnode.hpp"

namespace Element {

struct PreparedGraph
{
    GraphNode graph;
    explicit PreparedGraph (double sampleRate = 44100.0, int blockSize = 512)
    {
        graph.prepareToRender (sampleRate, blockSize);
    }
    
    ~PreparedGraph()
    {
        graph.releaseResources();
        graph.clear();
    }
};

}
