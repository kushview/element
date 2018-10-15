/*
    ContentComponent.h - This file is part of Element
    Copyright (c) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "engine/GraphNode.h"
#include "session/Session.h"

namespace Element {

class AppController;
class BreadCrumbComponent;
class ContentContainer;
class Globals;
class GraphEditorView;
class NavigationConcertinaPanel;
class Node;
class RackView;
class TransportBar;
class VirtualKeyboardView;

class ContentView : public Component,
                    public ApplicationCommandTarget,
                    public KeyListener
{
public:
    ContentView();
    virtual ~ContentView();

    virtual void initializeView (AppController&) { }
    virtual void willBeRemoved() { }
    virtual void willBecomeActive() { }
    virtual void didBecomeActive() { }
    virtual void stabilizeContent() { }

    /** Call this to disable the entire view if the app isn't unlocked */
    void disableIfNotUnlocked();

    inline void setEscapeTriggersClose (const bool shouldClose) { escapeTriggersClose = shouldClose; }
    
    virtual void getAllCommands (Array<CommandID>&) override { }
    virtual void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    virtual bool perform (const InvocationInfo&) override { return false; }

    /** @internal */
    ApplicationCommandTarget* getNextCommandTarget() override { return nextCommandTarget; }
    /** @internal */
    virtual void paint (Graphics& g) override;
    /** @internal */
    virtual bool keyPressed (const KeyPress& k, Component*) override;

private:
    bool escapeTriggersClose = false;
    ApplicationCommandTarget* nextCommandTarget = nullptr;
};

class ContentComponent :  public Component, 
                          public DragAndDropContainer,
                          public FileDragAndDropTarget,
                          public DragAndDropTarget,
                          public ApplicationCommandTarget
{
public:
    ContentComponent (AppController& app);
    ~ContentComponent() noexcept;

    NavigationConcertinaPanel* getNavigationConcertinaPanel() const { return nav.get(); }

    void childBoundsChanged (Component* child) override;
    void mouseDown (const MouseEvent&) override;
    void paint (Graphics &g) override;
    void resized() override;
    
    void setMainView (const String& name);
    void setAccessoryView (const String& name);
    String getMainViewName() const;
    void nextMainView();
    void backMainView();
    
    void saveState (PropertiesFile*);
    void restoreState (PropertiesFile*);
    
    int getNavSize();
    
    bool isVirtualKeyboardVisible() const { return virtualKeyboardVisible; }
    void setVirtualKeyboardVisible (const bool isVisible);
    void toggleVirtualKeyboard();
    VirtualKeyboardView* getVirtualKeyboardView() const { return keyboard.get(); }
    
    void setCurrentNode (const Node& node);
    void stabilize (const bool refreshDataPathTrees = false);
    
    void post (Message*);
    AppController& getAppController() { return controller; }
    Globals& getGlobals();
    SessionPtr getSession();
    
    bool isInterestedInFileDrag (const StringArray &files) override;
    void filesDropped (const StringArray &files, int x, int y) override;
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    void getAllCommands (Array<CommandID>&) override { }
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    bool perform (const InvocationInfo&) override { return false; }
    ApplicationCommandTarget* getNextCommandTarget() override;

private:
    AppController& controller;
    
    struct Tooltips;
    SharedResourcePointer<Tooltips> tips;
    
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
    
    String lastMainView;
    
    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
    void setContentView (ContentView* view, const bool accessory = false);
};

}
