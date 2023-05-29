/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <element/nodeobject.hpp>
#include <element/session.hpp>

#define EL_VIEW_GRAPH_MIXER "GraphMixerView"
#define EL_VIEW_CONSOLE     "LuaConsoleViw"

namespace element {

class ServiceManager;
class BreadCrumbComponent;
class ContentContainer;
class Context;
class GraphEditorView;
class NavigationConcertinaPanel;
class Node;
class RackView;
class TransportBar;
class VirtualKeyboardView;
class NodeChannelStripView;

using namespace juce;

class ContentView : public juce::Component,
                    public juce::ApplicationCommandTarget {
public:
    ContentView();
    virtual ~ContentView();

    virtual void initializeView (ServiceManager&) {}
    virtual void willBeRemoved() {}
    virtual void willBecomeActive() {}
    virtual void didBecomeActive() {}
    virtual void stabilizeContent() {}

    /** Save state to user settings */
    virtual void saveState (PropertiesFile*) {}

    /** Restore state from user settings */
    virtual void restoreState (PropertiesFile*) {}

    /** Get state attached to session */
    virtual void getState (String&) {}

    /** Apply state attached to session */
    virtual void setState (const String&) {}

    /** Set true if pressing escape should close the view */
    inline void setEscapeTriggersClose (const bool shouldClose) { escapeTriggersClose = shouldClose; }

    virtual void getAllCommands (Array<CommandID>&) override {}
    virtual void getCommandInfo (CommandID, ApplicationCommandInfo&) override {}
    virtual bool perform (const InvocationInfo&) override { return false; }

    Signal<void()> nodeMoved;

    /** @internal */
    ApplicationCommandTarget* getNextCommandTarget() override { return nextCommandTarget; }
    /** @internal */
    virtual void paint (Graphics& g) override;
    /** @internal */
    virtual bool keyPressed (const KeyPress& k) override;

private:
    bool escapeTriggersClose = false;
    ApplicationCommandTarget* nextCommandTarget = nullptr;
};

class ContentComponent : public Component,
                         public ApplicationCommandTarget,
                         public DragAndDropContainer,
                         public DragAndDropTarget,
                         public FileDragAndDropTarget {
protected:
    ContentComponent (ServiceManager& app);

public:
    virtual ~ContentComponent() noexcept;

    /** Creates an appropriate content component based on the product that is running */
    static ContentComponent* create (ServiceManager&);

    /** Post a message to the app controller */
    void post (Message*);

    /** Access to the app controller */
    ServiceManager& getServices() { return controller; }

    /** Access to global objects */
    Context& getGlobals();

    /** Access to the currently opened session */
    SessionPtr getSession();

    /** Change toolbar visibility */
    void setToolbarVisible (bool);

    /** Manually refresh the toolbar */
    void refreshToolbar();

    //=========================================================================
    /** Manually refresh the status bar */
    void refreshStatusBar();

    /** Chang statusbar visibility */
    void setStatusBarVisible (bool);

    /** Override this to resize the main content */
    virtual void resizeContent (const Rectangle<int>& area) { ignoreUnused (area); }

    virtual NavigationConcertinaPanel* getNavigationConcertinaPanel() const { return nullptr; }

    /** Implement if can set a view directly. Subclasses should take ownership
        of the view.
    */
    virtual void setMainView (ContentView* v)
    {
        jassertfalse;
        delete v;
    }

    virtual void setMainView (const String& name);
    virtual void setAccessoryView (const String& name);
    virtual String getMainViewName() const;
    virtual String getAccessoryViewName() const;

    virtual void nextMainView();
    virtual void backMainView();

    //=========================================================================
    virtual void saveState (PropertiesFile*);
    virtual void restoreState (PropertiesFile*);
    virtual void getSessionState (String&) {}
    virtual void applySessionState (const String&) {}

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

    //=========================================================================
    void setExtraView (Component* c);

    Component* getExtraView() const { return extra.get(); }

    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    bool isInterestedInFileDrag (const StringArray& files) override;
    /** @internal */
    void filesDropped (const StringArray& files, int x, int y) override;
    /** @internal */
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    /** @internal */
    void itemDropped (const SourceDetails& dragSourceDetails) override;

    /** @internal */
    void getAllCommands (Array<CommandID>&) override {}
    /** @internal */
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override {}
    /** @internal */
    bool perform (const InvocationInfo&) override { return false; }
    /** @internal */
    ApplicationCommandTarget* getNextCommandTarget() override;

private:
    ServiceManager& controller;

    struct Tooltips;
    SharedResourcePointer<Tooltips> tips;

    class Toolbar;
    friend class Toolbar;
    ScopedPointer<Toolbar> toolBar;

    class StatusBar;
    friend class StatusBar;
    ScopedPointer<StatusBar> statusBar;

    std::unique_ptr<Component> extra;
    int extraViewHeight = 44;

    bool statusBarVisible { true };
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
};

} // namespace element
