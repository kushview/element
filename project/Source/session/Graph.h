/*
    GraphModel.h - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "session/Node.h"

namespace Element {
    
class Graph : public Node
{
public:
    explicit Graph (const Identifier& type = Tags::graph);
    virtual ~Graph();
    
    void addPlugin();
    void addSubGraph (const Graph& graph);
};

}
