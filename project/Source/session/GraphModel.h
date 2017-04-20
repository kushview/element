/*
    GraphModel.h - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#ifndef EL_GRAPH_MODEL_H
#define EL_GRAPH_MODEL_H

#include "ElementApp.h"
#include "session/NodeModel.h"

namespace Element {
    
class Graph :  public Node
{
public:
    explicit Graph (const Identifier& type = Tags::graph);
    virtual ~Graph();
    
    void addPlugin();
    void addSubGraph (const Graph& graph);
};

typedef Graph GraphModel;

}

#endif // EL_GRAPH_MODEL_H
