#pragma once

#include "engine/GraphNode.h"

namespace Element {

class AudioProcessorNode : public GraphNode
{
public:
    AudioProcessorNode (uint32 nodeId, AudioProcessor* processor);
    ~AudioProcessorNode();
};

}
