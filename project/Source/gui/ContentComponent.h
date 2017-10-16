/*
    ContentComponent.h - This file is part of Element
    Copyright (c) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "engine/GraphNode.h"
#include "session/Session.h"

namespace Element {

class AppController;
class ContentContainer;
class Globals;
class GraphEditorView;
class NavigationConcertinaPanel;
class Node;
class RackView;
class TransportBar;

class ContentView : public Component {
public:
    virtual void willBecomeActive() { }
};

class ContentComponent :  public Component,
                          public DragAndDropContainer,
                          public FileDragAndDropTarget
{
public:
    ContentComponent (AppController& app);
    ~ContentComponent();

    void childBoundsChanged (Component* child) override;
    void mouseDown (const MouseEvent&) override;
    void paint (Graphics &g) override;
    void resized() override;

    void setContentView (ContentView* view);
    void setCurrentNode (const Node& node);
    void stabilize();
    
    void post (Message*);
    Globals& getGlobals();
    SessionRef getSession() const { return SessionRef(); }
    
    bool isInterestedInFileDrag (const StringArray &files) override;
    void filesDropped (const StringArray &files, int x, int y) override;

private:
    AppController& controller;
    ScopedPointer<NavigationConcertinaPanel> nav;
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
