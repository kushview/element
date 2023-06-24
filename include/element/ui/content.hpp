// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <memory>

#include <element/juce/core.hpp>
#include <element/juce/gui_basics.hpp>

#include <element/element.hpp>
#include <element/ui/about.hpp>
#include <element/ui/menumodels.hpp>

#include <element/processor.hpp>
#include <element/session.hpp>

#define EL_VIEW_GRAPH_MIXER "GraphMixerView"
#define EL_VIEW_CONSOLE     "LuaConsoleViw"

namespace element {

class Services;
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

class ContentView : public juce::Component {
public:
    ContentView();
    virtual ~ContentView();

    virtual void initializeView (Services&) {}
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

    // FIXME: this shouldn't exist here.
    Signal<void()> nodeMoved;

    /** @internal */
    virtual void paint (Graphics& g) override;
    /** @internal */
    virtual bool keyPressed (const KeyPress& k) override;

private:
    bool escapeTriggersClose = false;
};

class ContentComponent : public juce::Component,
                         public juce::DragAndDropContainer,
                         public juce::DragAndDropTarget,
                         public juce::FileDragAndDropTarget {
protected:
    ContentComponent (Context& app);

public:
    virtual ~ContentComponent() noexcept;

    /** Post a message to the app controller */
    void post (Message*);

    /** Access to the app controller */
    Services& services() { return controller; }

    /** Access to global objects */
    Context& context();

    /** Access to the currently opened session */
    SessionPtr session();

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

    //=========================================================================
    virtual void saveState (PropertiesFile*);
    virtual void restoreState (PropertiesFile*);
    virtual void getSessionState (String&) {}
    virtual void applySessionState (const String&) {}

    virtual void setNodeChannelStripVisible (const bool isVisible);
    virtual bool isNodeChannelStripVisible() const;

    virtual void setCurrentNode (const Node& node);
    virtual void stabilize (const bool refreshDataPathTrees = false);
    virtual void stabilizeViews();

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

private:
    Context& _context;
    Services& controller;

    struct Tooltips;
    SharedResourcePointer<Tooltips> tips;

    class Toolbar;
    friend class Toolbar;
    ScopedPointer<Toolbar> toolBar;

    class StatusBar;
    friend class StatusBar;
    ScopedPointer<StatusBar> statusBar;

    bool statusBarVisible { true };
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
};

class ContentFactory {
public:
    virtual ~ContentFactory() = default;

    /** Create a main content by type name.
        
        Return the content specified.  If type is empty or not supported,
        you should still return a valid Content object.  The object returned
        will be used as the content component in the main window.
    */
    virtual std::unique_ptr<ContentComponent> createMainContent (const juce::String& type) = 0;

    /** Create a menu bar model to use in the Main Window. */
    virtual std::unique_ptr<MainMenuBarModel> createMainMenuBarModel() { return nullptr; }

    /** Return a function to use when setting the Main Window's title. If this
        returns nullptr, Element will fallback to default titling.
     */
    virtual std::function<juce::String()> getMainWindowTitler() { return nullptr; }

    /** The struct returned will be used when showing the About Dialog inside Element. */
    virtual AboutInfo aboutInfo() { return {}; }

protected:
    ContentFactory() = default;

private:
    EL_DISABLE_COPY (ContentFactory)
};

} // namespace element
