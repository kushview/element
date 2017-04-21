
#include "session/Node.h"

namespace Element {
    struct NameSorter
    {
        NameSorter() { }
        int compareElements (const Node& lhs, const Node& rhs) {
            return lhs.getName() < rhs.getName() ? -1 :
            lhs.getName() == rhs.getName() ? 0 : 1;
        }
        friend class NodeArray;
    };
    
    const bool Node::canConnectTo (const Node& o) const
    {
        if (objectData.getParent() != o.objectData.getParent() ||
            objectData == o.objectData ||
            getNumAudioOuts() <= 0 || o.getNumAudioIns() <= 0)
        {
            return false;
        }
        
        return true;
    }
    
    ValueTree Node::getParentArcsNode() const
    {
        ValueTree arcs = objectData.getParent();
        if (arcs.hasType(Tags::nodes))
            arcs = arcs.getParent();
        if (! arcs.isValid())
            return ValueTree::invalid;
        
        jassert (arcs.hasType (Tags::node));
        return arcs.getOrCreateChildWithName (Tags::arcs, nullptr);
    }
    
    void Node::setMissingProperties()
    {
        objectData.getOrCreateChildWithName(Tags::nodes, nullptr);
        stabilizePropertyString (Slugs::type, "default");
        stabilizePropertyString (Slugs::name, "Default Node");
    }

    GraphNodePtr Node::getGraphNode() const
    {
        var val = objectData.getProperty ("object", var::null);
        return dynamic_cast<GraphNode*> (val.getObject());
    }
    
    void NodeArray::sortByName()
    {
        NameSorter sorter;
        this->sort (sorter);
    }
}
