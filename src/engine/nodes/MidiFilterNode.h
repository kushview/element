
#pragma once

#include "engine/GraphNode.h"

namespace Element {

class MidiFilterNode : public GraphNode
{
public:
    virtual ~MidiFilterNode();
    
protected:
    MidiFilterNode (uint32 nodeId);
    bool wantsMidiPipe() const override { return true; }
};

}
