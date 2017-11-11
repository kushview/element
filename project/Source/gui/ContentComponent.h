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
class VirtualKeyboardView;

class ContentView : public Component
{
public:
    virtual void willBecomeActive() { }
    virtual void didBecomeActive() { }
    virtual void stabilizeContent() { }
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
    
    void setMainView (const String& name);
    void setAccessoryView (const String& name);
    String getMainViewName() const;
    void nextMainView();
    
    int getNavSize();
    bool isVirtualKeyboardVisible() const { return virtualKeyboardVisible; }
    
    void setVirtualKeyboardVisible (const bool isVisible);
    void toggleVirtualKeyboard();
    
    void setCurrentNode (const Node& node);
    void stabilize();
    
    void post (Message*);
    AppController& getAppController() { return controller; }
    Globals& getGlobals();
    SessionPtr getSession();
    
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
    
    ScopedPointer<VirtualKeyboardView> keyboard;
        
    bool statusBarVisible;
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
    bool virtualKeyboardVisible = false;
    int virtualKeyboardSize = 80;
    
    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
    void setContentView (ContentView* view, const bool accessory = false);
};

}
