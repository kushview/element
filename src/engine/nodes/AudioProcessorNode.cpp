
#include "engine/nodes/AudioProcessorNode.h"

namespace Element {

AudioProcessorNode::AudioProcessorNode (uint32 nodeId, AudioProcessor* processor)
    : GraphNode (nodeId, processor)
{

}

AudioProcessorNode::~AudioProcessorNode()
{

}

}
