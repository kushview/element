/*
    ContentComponentPro.h - This file is part of Element
    Copyright (c) 2016-2019 Kushview, LLC.  All rights reserved.
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
class NodeChannelStripView;

class ContentComponentPro :  public Component, 
                          public DragAndDropContainer,
                          public FileDragAndDropTarget,
                          public DragAndDropTarget,
                          public ApplicationCommandTarget
{
public:
    ContentComponentPro (AppController& app);
    ~ContentComponentPro() noexcept;

    NavigationConcertinaPanel* getNavigationConcertinaPanel() const { return nav.get(); }

    void childBoundsChanged (Component* child) override;
    void mouseDown (const MouseEvent&) override;
    void paint (Graphics &g) override;
    void resized() override;
    
    void setMainView (const String& name);
    void setAccessoryView (const String& name);
    String getMainViewName() const;
    String getAccessoryViewName() const;

    void nextMainView();
    void backMainView();
    
    void saveState (PropertiesFile*);
    void restoreState (PropertiesFile*);
    
    int getNavSize();
    
    bool isVirtualKeyboardVisible() const { return virtualKeyboardVisible; }
    void setVirtualKeyboardVisible (const bool isVisible);
    void toggleVirtualKeyboard();
    VirtualKeyboardView* getVirtualKeyboardView() const { return keyboard.get(); }
    
    void setNodeChannelStripVisible (const bool isVisible);
    bool isNodeChannelStripVisible() const;

    void setCurrentNode (const Node& node);
    void stabilize (const bool refreshDataPathTrees = false);
    void stabilizeViews();

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

    void setShowAccessoryView (const bool show);
    bool showAccessoryView() const;

private:
    AppController& controller;
    
    struct Tooltips;
    SharedResourcePointer<Tooltips> tips;
    ScopedPointer<NavigationConcertinaPanel> nav;
    ScopedPointer<ContentContainer> container;
    StretchableLayoutManager layout;
    
    class Resizer; friend class Resizer;
    ScopedPointer<Resizer> bar1;
    
    class Toolbar; friend class Toolbar;
    ScopedPointer<Toolbar> toolBar;
    
    class StatusBar; friend class StatusBar;
    ScopedPointer<StatusBar> statusBar;
    
    ScopedPointer<VirtualKeyboardView> keyboard;
    ScopedPointer<NodeChannelStripView> nodeStrip;

    bool statusBarVisible;
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
    bool virtualKeyboardVisible = false;
    int virtualKeyboardSize = 80;
    int nodeStripSize = 90;
    
    String lastMainView;
    
    void resizerMouseDown();
    void resizerMouseUp();
    void updateLayout();
    void setContentView (ContentView* view, const bool accessory = false);
};

}
