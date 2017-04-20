/*
    NodeModel.h - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_NODE_MODEL_H
#define ELEMENT_NODE_MODEL_H

#include "ElementApp.h"

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
    
    const Identifier getNodeType() const { return Identifier (getProperty(Slugs::type).toString()); }
    const bool hasNodeType (const Identifier& t) const { return getNodeType() == t; }
    const String getName() const { return getProperty (Slugs::name); }
    
private:
    inline void setMissingProperties()
    {
        stabilizePropertyString (Slugs::type, "default");
        stabilizePropertyString (Slugs::name, "Default Node");
    }
};

typedef Node NodeModel;

class NodeArray : public Array<Node> { };

}

#endif // ELEMENT_NODE_MODEL_H
