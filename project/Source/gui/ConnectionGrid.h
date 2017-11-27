/*
    This file is part of the element modules for the JUCE Library
    Copyright (C) 2016  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "gui/ContentComponent.h"

namespace Element {

class BreadCrumbComponent;
class Node;

class ConnectionGrid : public ContentView,
                       public DragAndDropTarget
{
public:
    ConnectionGrid();
    ~ConnectionGrid();
    
    void setNode (const Node& node);

    void didBecomeActive() override;

    void paint (Graphics&) override;
    void resized() override;
    
    void mouseDown (const MouseEvent& ev) override;
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    
#if 0
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override;
#endif
    
private:
    ScopedPointer<BreadCrumbComponent> breadcrumb;
    
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
