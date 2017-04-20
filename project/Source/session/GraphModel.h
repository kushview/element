/*
    GraphModel.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_GRAPH_MODEL_H
#define ELEMENT_GRAPH_MODEL_H

#include "session/BlockModel.h"

namespace Element {
    
class GraphModel :  public BlockModel
{
public:
    GraphModel();
    ~GraphModel();
    void addPlugin();
    void addSubGraph (const GraphModel& graph);
};

}

#endif // ELEMENT_GRAPH_MODEL_H
