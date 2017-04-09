/*
    ContentComponent.h - This file is part of Element
    Copyright (c) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#ifndef EL_CONTENT_COMPONENT_H
#define EL_CONTENT_COMPONENT_H

#include "engine/GraphNode.h"

namespace Element {

class ContentContainer;
class GuiApp;
class GraphEditorView;
class RackView;
class TransportBar;

class ContentComponent :  public Component,
                          public DragAndDropContainer,
                          private Timer
{
public:
    ContentComponent (GuiApp& app);
    ~ContentComponent();

    void childBoundsChanged (Component* child) override;
    void paint (Graphics &g) override;
    void resized() override;

    void setRackViewComponent (Component* comp);
    void setRackViewNode (GraphNodePtr node);
    void stabilize();
    
    GuiApp& app();

private:
    GuiApp& gui;
    ScopedPointer<ContentContainer> top;
    ScopedPointer<TransportBar>  transport;
    ScopedPointer<TooltipWindow> toolTips;
    StretchableLayoutManager layoutVertical;
    ScopedPointer<StretchableLayoutResizerBar> bar1;
    ScopedPointer<RackView> rack;
    Shared<Monitor> playbackMonitor;
    
    friend class Timer;
    void timerCallback() override;
};

}

#endif // ELEMENT_CONTENT_COMPONENT_H
