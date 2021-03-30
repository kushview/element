
#pragma once

#include "graph/BaseNode.h"

namespace Element {

/** Base class for all graph types */
class GraphNode : public BaseNode
{
public:
    GraphNode() : BaseNode() {}
    virtual ~GraphNode() = default;
};

}
