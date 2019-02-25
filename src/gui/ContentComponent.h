/*
    ContentComponent.h - This file is part of Element
    Copyright (c) 2016-2019 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "engine/GraphNode.h"
#include "session/Session.h"
#include "Workspace.h"

#define EL_VIEW_GRAPH_MIXER "GraphMixerView"

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
protected:
    ContentComponent (AppController& app);

public:
    virtual ~ContentComponent() noexcept;

    /** Creates an appropriate content component based on the product that is running */
    static ContentComponent* create (AppController&);

    /** Post a message to the app controller */
    void post (Message*);

    /** Access to the app controller */
    AppController& getAppController() { return controller; }

    /** Access to global objects */
    Globals& getGlobals();

    /** Access to the currently opened session */
    SessionPtr getSession();

    /** Manually refresh the toolbar */
    void refreshToolbar();
    
    /** Manually refresh the status bar */
    void refreshStatusBar();

    /** Override this to resize the main content */
    virtual void resizeContent (const Rectangle<int>& area) { ignoreUnused (area); }

    virtual NavigationConcertinaPanel* getNavigationConcertinaPanel() const { return nullptr; }
    
    virtual void setMainView (const String& name);
    virtual void setAccessoryView (const String& name);
    virtual String getMainViewName() const;
    virtual String getAccessoryViewName() const;

    virtual void nextMainView();
    virtual void backMainView();
    
    virtual void saveState (PropertiesFile*);
    virtual void restoreState (PropertiesFile*);
    
    virtual int getNavSize();
    
    virtual bool isVirtualKeyboardVisible() const { return false; }
    virtual void setVirtualKeyboardVisible (const bool isVisible);
    virtual void toggleVirtualKeyboard();
    virtual VirtualKeyboardView* getVirtualKeyboardView() const { return nullptr; }
    
    virtual void setNodeChannelStripVisible (const bool isVisible);
    virtual bool isNodeChannelStripVisible() const;

    virtual void setCurrentNode (const Node& node);
    virtual void stabilize (const bool refreshDataPathTrees = false);
    virtual void stabilizeViews();

    virtual void setShowAccessoryView (const bool show);
    virtual bool showAccessoryView() const;

    virtual String getWorkspaceName() const { return String(); }
    virtual WorkspaceState getWorkspaceState() { return { }; }
    virtual void applyWorkspaceState (const WorkspaceState&) { }
    virtual void addWorkspaceItemsToMenu (PopupMenu&) {}
    virtual void handleWorkspaceMenuResult (int) {}

    /** @internal */
    void paint (Graphics &g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    bool isInterestedInFileDrag (const StringArray &files) override;
    /** @internal */
    void filesDropped (const StringArray &files, int x, int y) override;
    /** @internal */
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    /** @internal */
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    /** @internal */
    void getAllCommands (Array<CommandID>&) override { }
    /** @internal */
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    /** @internal */
    bool perform (const InvocationInfo&) override { return false; }
    /** @internal */
    ApplicationCommandTarget* getNextCommandTarget() override;

private:
    AppController& controller;
    
    struct Tooltips;
    SharedResourcePointer<Tooltips> tips;
    
    class Toolbar; friend class Toolbar;
    ScopedPointer<Toolbar> toolBar;
    
    class StatusBar; friend class StatusBar;
    ScopedPointer<StatusBar> statusBar;

    bool statusBarVisible;
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
};

}
