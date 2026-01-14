// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <element/element.hpp>

#include <element/juce/core.hpp>
#include <element/juce/gui_basics.hpp>

#include <element/ui/about.hpp>
#include <element/ui/mainwindow.hpp>
#include <element/ui/menumodels.hpp>
#include <element/ui/preferences.hpp>
#include <element/ui/view.hpp>

#include <element/session.hpp>
#include <element/signals.hpp>

namespace element {

class Services;
class Context;
class MainWindow;
class Node;

//==============================================================================
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
    virtual void saveState (juce::PropertiesFile*) {}

    /** Restore state from user settings */
    virtual void restoreState (juce::PropertiesFile*) {}

    /** Get state attached to session */
    virtual void getState (juce::String&) {}

    /** Apply state attached to session */
    virtual void setState (const juce::String&) {}

    /** Set true if pressing escape should close the view */
    inline void setEscapeTriggersClose (const bool shouldClose) { escapeTriggersClose = shouldClose; }

    // FIXME: nodeMoved signal shouldn't exist here.
    Signal<void()> nodeMoved;

    /** @internal */
    virtual void paint (juce::Graphics& g) override;
    /** @internal */
    virtual bool keyPressed (const juce::KeyPress& k) override;

private:
    bool escapeTriggersClose = false;
};

//==============================================================================
class Content : public juce::Component {
protected:
    Content (Context& ctx);

public:
    virtual ~Content() noexcept;

    /** Access to global objects */
    Context& context();

    /** Access to the app controller */
    Services& services() { return _services; }

    /** Access to the currently opened session */
    SessionPtr session();

    /** Post a message to Services */
    void post (juce::Message*);

    //=========================================================================
    /** Change toolbar visibility */
    void setToolbarVisible (bool);

    /** Manually refresh the toolbar */
    void refreshToolbar();

    //=========================================================================
    /** Manually refresh the status bar */
    void refreshStatusBar();

    /** Chang statusbar visibility */
    void setStatusBarVisible (bool);

    //=========================================================================
    virtual void presentView (std::unique_ptr<View>) = 0;
    virtual void presentView (const juce::String&) = 0;

    //=========================================================================
    virtual void saveState (juce::PropertiesFile*);
    virtual void restoreState (juce::PropertiesFile*);
    virtual void getSessionState (juce::String&) {}
    virtual void applySessionState (const juce::String&) {}

    virtual void setCurrentNode (const Node& node);
    virtual void stabilize (const bool refreshDataPathTrees = false);
    virtual void stabilizeViews();

    /** @internal */
    void paint (juce::Graphics& g) override;
    /** @internal */
    void resized() override;

protected:
    /** Override this to resize the main content */
    virtual void resizeContent (const juce::Rectangle<int>& area) { ignoreUnused (area); }

private:
    Context& _context;
    Services& _services;

    struct Tooltips;
    juce::SharedResourcePointer<Tooltips> tips;

    class Toolbar;
    friend class Toolbar;
    std::unique_ptr<Toolbar> toolBar;

    class StatusBar;
    friend class StatusBar;
    std::unique_ptr<StatusBar> statusBar;

    bool statusBarVisible { true };
    int statusBarSize;
    bool toolBarVisible;
    int toolBarSize;
};

//==============================================================================
class ContentFactory {
public:
    virtual ~ContentFactory() = default;

    /** Create a main content by type name.
        
        Return the content specified.  If type is empty or not supported,
        you should still return a valid Content object.  The object returned
        will be used as the content component in the main window.
    */
    virtual std::unique_ptr<Content> createMainContent (const juce::String& type) = 0;

    /** Create a menu bar model to use in the Main Window. */
    virtual std::unique_ptr<MainMenuBarModel> createMainMenuBarModel() { return nullptr; }

    /** Return a function to use when setting the Main Window's title. If this
        returns nullptr, Element will fallback to default titling.
     */
    virtual std::function<juce::String()> getMainWindowTitler() { return nullptr; }

    /** Create a custom MainWindow to be used by the UI service. */
    virtual std::unique_ptr<MainWindow> createMainWindow() { return nullptr; }

    /** Create a custom Preferences Widget to use by the UI service. */
    virtual std::unique_ptr<Preferences> createPreferences() { return nullptr; }

    /** The struct returned will be used when showing the About Dialog inside Element. */
    virtual AboutInfo aboutInfo() { return {}; }

protected:
    ContentFactory() = default;

private:
    EL_DISABLE_COPY (ContentFactory)
};

} // namespace element
