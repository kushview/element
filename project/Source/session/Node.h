/*
 NodeModel.h - This file is part of Element
 Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
 */

#ifndef EL_NODE_H
#define EL_NODE_H

#include "ElementApp.h"
#include "engine/GraphNode.h"
namespace Element {
    
    class Node : public ObjectModel
    {
    public:
        Node() : ObjectModel (Tags::node)
        {
            setMissingProperties();
        }
        
        Node (const ValueTree& data, const bool setMissing = true)
            : ObjectModel (data)
        {
            jassert (data.hasType (Tags::node));
            if (setMissing)
                setMissingProperties();
        }
        
        Node (const Identifier& nodeType)
            : ObjectModel (Tags::node)
        {
            objectData.setProperty (Slugs::type, nodeType.toString(), nullptr);
            setMissingProperties();
        }
        
        const uint32 getNodeId() const { return (uint32)(int64) getProperty ("id"); }
        const Identifier getNodeType() const { return Identifier (getProperty(Slugs::type).toString()); }
        const bool hasNodeType (const Identifier& t) const { return getNodeType() == t; }
        const String getName() const { return getProperty (Slugs::name); }
        
        GraphNodePtr getGraphNode() const;
        const int getNumAudioIns()  const { return (int) getProperty ("numAudioIns", 0); }
        const int getNumAudioOuts() const { return (int) getProperty ("numAudioOuts", 0); }
        
        const bool canConnectTo (const Node& o) const;
        ValueTree getNodesValueTree() const { return objectData.getChildWithName(Tags::nodes); }
        ValueTree getParentArcsNode() const;
        
    private:
        void setMissingProperties();
    };
    
    typedef Node NodeModel;
    
    class NodeArray : public Array<Node> {
    public:
        void sortByName();
        
    };
    
}

#endif // EL_NODE_H
