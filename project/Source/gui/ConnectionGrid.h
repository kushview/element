/*
    This file is part of the element modules for the JUCE Library
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.
*/

#ifndef EL_CONNECTION_GRID_H
#define EL_CONNECTION_GRID_H

#include "ElementApp.h"

namespace Element {
    
class Node;

class ConnectionGrid : public Component,
                       public DragAndDropTarget
{
public:
    ConnectionGrid();
    ~ConnectionGrid();
    
    void setGraphNode (const Node& node);
    
    void paint (Graphics&) override;
    void resized() override;
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    
#if 0
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override;
#endif
    
private:
    friend class PatchMatrix;
    class PatchMatrix; PatchMatrix* matrix;
    
    friend class Controls;
    class Controls; Controls* controls;
    
    friend class Sources;
    class Sources; Sources* sources;
    
    friend class Destinations;
    class Destinations; Destinations* destinations;
    
    friend class Quads;
    class Quads; ScopedPointer<QuadrantLayout> quads;
};

}

#endif  // EL_CONNECTION_GRID_H
