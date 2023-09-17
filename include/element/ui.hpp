// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/about.hpp>
#include <element/session.hpp>
#include <element/services.hpp>
#include <element/signals.hpp>
#include <element/ui/designer.hpp>
#include <element/ui/about.hpp>

namespace element {

class Commands;
class Content;
class ContentFactory;
class Context;
class LookAndFeel_E1;
class MainWindow;
class PluginWindow;
class Services;
class WindowManager;

class GuiService : public Service,
                   public juce::ApplicationCommandTarget,
                   private juce::ChangeListener {
public:
    Signal<void()> nodeSelected;
    Signal<void()> sigRefreshed;

    GuiService (Context& w, Services& a);
    ~GuiService();

    void activate() override;
    void deactivate() override;
    bool handleMessage (const AppMessage&) override;
    void shutdown() override;

    void run();
    Commands& commands();

    /** Check for a newer version and show alert, if available. */
    void checkUpdates();

    /** Launch the updater tool, if available and enabled. */
    void launchUpdater();

    Services& services() const { return controller; }
    KeyListener* getKeyListener() const;

    void closeAllWindows();

    MainWindow* getMainWindow() const noexcept;
    void refreshMainMenu();

    void showPreferencesDialog (const String& section = {});

    void runDialog (const String& uri);
    void runDialog (Component* c, const String& title = String());

    /** Get a reference to Sesison data */
    SessionRef session();

    /** Show plugin windows for a node */
    void showPluginWindowsFor (const Node& node,
                               const bool recursive = true,
                               const bool force = false,
                               const bool focus = false);

    /** present a plugin window */
    void presentPluginWindow (const Node& node, const bool focus = false);

    /** Sync all UI elements with application/plugin */
    void stabilizeContent();

    /** Stabilize Views Only */
    void stabilizeViews();

    /** Refershes the system tray based on Settings */
    void refreshSystemTray();

    bool haveActiveWindows() const;

    /* Command manager... */
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array<CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;

    /** Returns the content component for this instance */
    Content* content();

    int getNumPluginWindows() const;
    PluginWindow* getPluginWindow (const int window) const;
    PluginWindow* getPluginWindow (const Node& node) const;

    /** Close all plugin windows housed by this controller */
    void closeAllPluginWindows (const bool windowVisible = true);

    /** Close plugin windows for a Node ID
     
        @param nodeId   The Node to close windows for
        @param visible  The visibility state flag, true indicates the window should be open when loaded
    */
    void closePluginWindowsFor (uint32 nodeId, const bool windowVisible);

    /** Close plugin windows for a Node */
    void closePluginWindowsFor (const Node& node, const bool windowVisible);

    /** @internal close a specific plugin window
        PluginWindows call this when they need deleted
    */
    void closePluginWindow (PluginWindow*);

    /** Get the look and feel used by this instance */
    element::LookAndFeel_E1& getLookAndFeel();

    /** Clears the current content component */
    void clearContentComponent();

    // TODO: content manager on selected nodes
    Node getSelectedNode() const { return selectedNode; }
    // TODO: content manager on selected nodes.
    // WARNING: don't call from outside the main thread.
    void selectNode (const Node& node)
    {
        if (selectedNode == node)
            return;
        selectedNode = node;
        nodeSelected();
    }

    void checkForegroundStatus();

    void setContentFactory (std::unique_ptr<ContentFactory>);

    using RecentFiles = juce::RecentlyOpenedFilesList;
    RecentFiles& recentFiles();

private:
    class UpdateManager;
    std::unique_ptr<UpdateManager> updates;
    class Impl;
    std::unique_ptr<Impl> impl;

    Services& controller;
    Context& world;
    SessionRef sessionRef;
    OwnedArray<PluginWindow> pluginWindows;

    std::unique_ptr<WindowManager> windowManager;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<Content> _content;
    std::unique_ptr<DialogWindow> about;
    std::unique_ptr<ContentFactory> factory;
    std::unique_ptr<Designer> designer;

    Node selectedNode; // TODO: content manager

    struct KeyPressManager;
    std::unique_ptr<KeyPressManager> keys;

    AboutInfo appInfo;
    struct ForegroundCheck : public Timer {
        ForegroundCheck (GuiService& _ui) : ui (_ui) {}
        void timerCallback() override;
        GuiService& ui;
    } foregroundCheck;

    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;

    void showSplash();
    void toggleAboutScreen();

    void saveProperties (PropertiesFile* props);
    void setMainWindowTitler (std::function<juce::String()>);
};

// TODO:
using UI = GuiService;

} // namespace element
