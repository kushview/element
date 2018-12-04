
#include "engine/nodes/MidiFilterNode.h"
#include "engine/PlaceholderProcessor.h"

namespace Element {

MidiFilterNode::MidiFilterNode (uint32 nodeId)
    : GraphNode (nodeId, new PlaceholderProcessor (0, 0, true, true)) { }

MidiFilterNode::~MidiFilterNode()
{

}

}
