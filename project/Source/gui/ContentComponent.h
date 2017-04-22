/*
    ContentComponent.h - This file is part of Element
    Copyright (c) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#ifndef EL_CONTENT_COMPONENT_H
#define EL_CONTENT_COMPONENT_H

#include "engine/GraphNode.h"
#include "session/Session.h"

namespace Element {

class AppController;
class ContentContainer;
class Globals;
class GuiApp;
class GraphEditorView;
class RackView;
class TransportBar;

class ContentComponent :  public Component,
                          public DragAndDropContainer
{
public:
    ContentComponent (AppController& app, GuiApp& gui);
    ~ContentComponent();

    void childBoundsChanged (Component* child) override;
    void mouseDown (const MouseEvent&) override;
    void paint (Graphics &g) override;
    void resized() override;

    void setRackViewComponent (Component* comp);
    void setRackViewNode (GraphNodePtr node);
    void stabilize();
    
    void post (Message*);
    Globals& getGlobals();
    SessionRef getSession() const { return SessionRef(); }
    
    JUCE_DEPRECATED(GuiApp& app());

private:
    AppController& controller;
    GuiApp& gui;
    ScopedPointer<Component> nav;
    ScopedPointer<ContentContainer> container;
    ScopedPointer<TooltipWindow> toolTips;
    StretchableLayoutManager layout;
    
    class Resizer; friend class Resizer;
    ScopedPointer<Resizer> bar1;
    
    class Toolbar; friend class Toolbar;
    ScopedPointer<Toolbar> toolBar;
    
    class StatusBar; friend class StatusBar;
    ScopedPointer<StatusBar> statusBar;
    
    bool statusBarVisible;
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
    
    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
};

}

#endif // ELEMENT_CONTENT_COMPONENT_H
