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

#include <element/audioengine.hpp>
#include <element/devices.hpp>
#include <element/engine.hpp>
#include <element/services.hpp>

#include <element/ui.hpp>
#include <element/ui/content.hpp>
#include <element/ui/updater.hpp>

#include "services/sessionservice.hpp"
#include "gui/views/VirtualKeyboardView.h"
#include "gui/AboutComponent.h"
#include "gui/GuiCommon.h"
#include <element/ui/style.hpp>
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "gui/PreferencesComponent.h"
#include "gui/SystemTray.h"
#include "gui/WindowManager.h"
#include <element/ui/standard.hpp>
#include "gui/capslock.hpp"

#include <element/ui/commands.hpp>

#include "version.hpp"

namespace element {

//=============================================================================
class DefaultContentFactory : public ContentFactory
{
public:
    DefaultContentFactory (Context& ctx)
        : context (ctx) {}
    ~DefaultContentFactory() {}

    std::unique_ptr<ContentComponent> createMainContent (const String& type) override
    {
        return std::make_unique<StandardContentComponent> (context);
    }

private:
    Context& context;
};

//=============================================================================
struct GlobalLookAndFeel
{
    GlobalLookAndFeel() { LookAndFeel_E1::setDefaultLookAndFeel (&look); }
    ~GlobalLookAndFeel() { LookAndFeel_E1::setDefaultLookAndFeel (nullptr); }
    element::LookAndFeel_E1 look;
};

/** Dispatches global key presses */
struct GuiService::KeyPressManager : public KeyListener
{
    KeyPressManager (GuiService& g) : owner (g) {}
    ~KeyPressManager() {}

    bool keyPressed (const KeyPress& key, Component* component) override
    {
        bool handled = false;
        if (isCapsLockOn() && isVirtualKeyboardVisible())
            handled = handleVirtualKeyboardPressed (key, component);

        if (! handled)
            handled = handleGraphChange (key, component);

        return handled;
    }

    bool keyStateChanged (bool isKeyDown, Component* component) override
    {
        bool handled = false;
        if (isCapsLockOn() && isVirtualKeyboardVisible())
            handled = handleVirtualKeyboardStateChange (isKeyDown, component);
        return handled;
    }

private:
    bool isVirtualKeyboardVisible() const
    {
        if (auto* cc = owner.getContentComponent())
            return cc->isVirtualKeyboardVisible();
        return false;
    }

    VirtualKeyboardView* getVirtualKeyboardView() const
    {
        if (auto* cc = owner.getContentComponent())
            return cc->getVirtualKeyboardView();
        return nullptr;
    }

    bool handleVirtualKeyboardPressed (const KeyPress& key, Component*)
    {
        if (auto* vcv = getVirtualKeyboardView())
            return vcv->keyPressed (key);
        return false;
    }

    bool handleVirtualKeyboardStateChange (bool isKeyDown, Component*)
    {
        if (auto* vcv = getVirtualKeyboardView())
            return vcv->keyStateChanged (isKeyDown);
        return false;
    }

    bool handleGraphChange (const KeyPress& key, Component*)
    {
        if (auto sess = owner.session())
            for (int i = 0; i < sess->getNumGraphs(); ++i)
                if (tryGraphKeyPress (sess, sess->getActiveGraph(), sess->getGraph (i), key))
                    return true;

        return false;
    }

    bool tryGraphKeyPress (SessionPtr session, const Node& active, const Node& g, const KeyPress& key)
    {
        if (active == g)
            return false;

        const auto gp = KeyPress::createFromDescription (g.getProperty (tags::keyMap));
        if (gp.isValid() && key == gp)
        {
            owner.closeAllPluginWindows (true);
            auto graphs = session->data().getChildWithName (tags::graphs);
            graphs.setProperty (tags::active, graphs.indexOf (g.data()), 0);
            owner.sibling<EngineService>()->setRootNode (g);
            owner.showPluginWindowsFor (g, true, false, false);
            return true;
        }

        return false;
    }

    GuiService& owner;
};

static std::unique_ptr<GlobalLookAndFeel> sGlobalLookAndFeel;
static Array<GuiService*> sGuiControllerInstances;

class GuiService::UpdateManager
{
public:
    UpdateManager()
    {
        setupUpdater();
    }

    ~UpdateManager()
    {
        _conn.disconnect();
    }

    bool launchRequested() const noexcept { return launchUpdaterOnExit; }
    void launchDetached()
    {
        updater.launch();
    }

    void check() { updater.check (true); }

private:
    void setupUpdater()
    {
        juce::String ver (EL_VERSION_STRING);
        ver << "-" << EL_BUILD_NUMBER;
        // ver = "0.20.0.0";
        updater.setInfo ("net.kushview.element", ver.toStdString());
        updater.setRepository ("https://cd.kushview.net/element/release");
#if JUCE_MAC
        updater.setRepository (updater.repository() + "/osx");
#elif JUCE_WINDOWS
        updater.setRepository (updater.repository() + "/windows");
#else
        updater.setRepository (updater.repository() + "/linux");
#endif
        _conn.disconnect();
        _conn = updater.sigUpdatesAvailable.connect ([this]() {
            if (updater.available().size() > 0)
            {
                auto res = AlertWindow::showYesNoCancelBox (
                    AlertWindow::InfoIcon,
                    "Updates Ready",
                    "There are updates ready.  Would you like to quit Element and launch the Updater?");
                if (res == 1)
                {
                    if (! updater.exists())
                    {
                        AlertWindow::showMessageBoxAsync (
                            AlertWindow::WarningIcon,
                            "Updates",
                            "Could not find the updater program on your system.");
                    }
                    else
                    {
                        launchUpdaterOnExit = true;
                        juce::JUCEApplication::getInstance()->quit();
                    }
                }
            }
            else
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                                  "Updates",
                                                  "You're up to date with the latest Element");
            }
        });
    }

    bool launchUpdaterOnExit { false };
    ui::Updater updater;
    boost::signals2::connection _conn;
};

//=============================================================================
void GuiService::ForegroundCheck::timerCallback()
{
    static bool sIsForeground = true;
    auto foreground = Process::isForegroundProcess();
    if (sIsForeground == foreground)
        return;

    if (! ui.settings().hidePluginWindowsWhenFocusLost())
        return;

    auto session = ui.context().session();
    jassert (session);
    if (foreground)
    {
        if (session)
            ui.showPluginWindowsFor (session->getCurrentGraph(), true, false);
        ui.getMainWindow()->toFront (true);
    }
    else if (! foreground)
    {
        ui.closeAllPluginWindows();
    }

    sIsForeground = foreground;
    stopTimer();
}

void GuiService::checkForegroundStatus()
{
    if (getRunMode() != RunMode::Standalone || foregroundCheck.isTimerRunning())
        return;
    foregroundCheck.startTimer (50);
}

//=============================================================================
class GuiService::Impl
{
public:
    Impl (GuiService& gs)
        : gui (gs) {}

    void restoreRecents()
    {
        const auto recentList = DataPath::applicationDataDir().getChildFile ("recents.txt");
        if (recentList.existsAsFile())
        {
            FileInputStream stream (recentList);
            recents.restoreFromString (stream.readEntireStreamAsString());
        }
    }

    void saveRecents()
    {
        const auto recentList = DataPath::applicationDataDir().getChildFile ("recents.txt");
        if (! recentList.existsAsFile())
            recentList.create();
        if (recentList.exists())
            recentList.replaceWithText (recents.toString(), false, false);
    }

private:
    friend class GuiService;
    GuiService& gui;
    Commands commands;
    juce::RecentlyOpenedFilesList recents;
    juce::UndoManager undo;
    juce::File lastSavedFile;
    juce::File lastExportedGraph;
};

//=============================================================================
void GuiService::setContentFactory (std::unique_ptr<ContentFactory> newDecorator)
{
    clearContentComponent();
    if (mainWindow)
        mainWindow->windowTitleFunction = nullptr;
    if (factory)
        factory.reset();
    factory = std::move (newDecorator);
    if (! factory)
        factory = std::make_unique<DefaultContentFactory> (context());
    appInfo = factory->aboutInfo();
}

//=============================================================================
GuiService::GuiService (Context& w, Services& a)
    : Service(),
      controller (a),
      world (w),
      windowManager (nullptr),
      mainWindow (nullptr),
      foregroundCheck (*this)
{
    impl = std::make_unique<Impl> (*this);
    factory = std::make_unique<DefaultContentFactory> (w);
    updates = std::make_unique<UpdateManager>();
    keys.reset (new KeyPressManager (*this));
    windowManager.reset (new WindowManager (*this));

    auto& commands = impl->commands;
    commands.registerAllCommandsForTarget (this);
    commands.setFirstCommandTarget (this);

    if (sGlobalLookAndFeel == nullptr)
        sGlobalLookAndFeel = std::make_unique<GlobalLookAndFeel>();
    sGuiControllerInstances.add (this);
}

GuiService::~GuiService()
{
    updates.reset();
    sGuiControllerInstances.removeFirstMatchingValue (this);
    if (sGuiControllerInstances.size() <= 0)
        sGlobalLookAndFeel.reset();
}

element::LookAndFeel_E1& GuiService::getLookAndFeel()
{
    jassert (sGlobalLookAndFeel);
    return sGlobalLookAndFeel->look;
}

void GuiService::saveProperties (PropertiesFile* props)
{
    jassert (props);

    if (mainWindow)
    {
        props->setValue ("mainWindowState", mainWindow->getWindowStateAsString());
        props->setValue ("mainWindowFullScreen", mainWindow->isFullScreen());
        props->setValue ("mainWindowVisible", mainWindow->isOnDesktop() && mainWindow->isVisible());
    }

    if (content)
    {
        props->setValue ("lastContentView", content->getMainViewName());
        props->setValue ("navSize", content->getNavSize());
        props->setValue ("virtualKeyboard", content->isVirtualKeyboardVisible());
        props->setValue ("channelStrip", content->isNodeChannelStripVisible());
        props->setValue ("accessoryView", content->showAccessoryView());
        content->saveState (props);
    }
}

void GuiService::activate()
{
    context().devices().addChangeListener (this);
    impl->restoreRecents();
}

void GuiService::deactivate()
{
    context().devices().removeChangeListener (this);
    nodeSelected.disconnect_all_slots();

    saveProperties (settings().getUserSettings());

    closeAllPluginWindows (true);
    SystemTray::setEnabled (false);

    if (mainWindow)
    {
        if (keys)
            mainWindow->removeKeyListener (keys.get());

        closeAllWindows();
        mainWindow->setVisible (false);
        mainWindow->removeFromDesktop();
        mainWindow = nullptr;
    }

    keys = nullptr;

    if (windowManager)
    {
        windowManager = nullptr;
    }

    if (content)
    {
        content = nullptr;
    }

    impl->saveRecents();
}

void GuiService::closeAllWindows()
{
    if (! windowManager)
        return;
    windowManager->closeAll();
}

Commands& GuiService::commands() { return impl->commands; }

void GuiService::checkUpdates() { updates->check(); }

void GuiService::runDialog (const String& uri)
{
    if (uri == "preferences")
    {
        if (auto* const dialog = windowManager->findDialogByName ("Preferences"))
        {
            if (! dialog->isOnDesktop() || ! dialog->isVisible())
            {
                dialog->setVisible (true);
                dialog->addToDesktop();
            }
            dialog->toFront (true);
            return;
        }

        DialogOptions opts;
        opts.content.set (new PreferencesComponent (world, *this), true);
        opts.useNativeTitleBar = true;
        opts.dialogTitle = "Preferences";
        opts.componentToCentreAround = (Component*) mainWindow.get();

        if (DialogWindow* dw = opts.create())
        {
            dw->setName ("Preferences");
            dw->setComponentID ("PreferencesDialog");
            windowManager->push (dw, true);
        }
    }
}

void GuiService::closePluginWindow (PluginWindow* w)
{
    if (windowManager)
        windowManager->closePluginWindow (w);
}
void GuiService::closePluginWindowsFor (uint32 nodeId, const bool visible)
{
    if (windowManager)
        windowManager->closeOpenPluginWindowsFor (nodeId, visible);
}
void GuiService::closeAllPluginWindows (const bool visible)
{
    if (windowManager)
        windowManager->closeAllPluginWindows (visible);
}

void GuiService::closePluginWindowsFor (const Node& node, const bool visible)
{
    if (! node.isGraph() && windowManager)
        windowManager->closeOpenPluginWindowsFor (node, visible);
}

void GuiService::runDialog (Component* c, const String& title)
{
    DialogOptions opts;
    opts.content.set (c, true);
    opts.dialogTitle = title.isNotEmpty() ? title : c->getName();
    opts.componentToCentreAround = (Component*) mainWindow.get();
    if (windowManager)
        if (DialogWindow* dw = opts.create())
            windowManager->push (dw);
}

void GuiService::showSplash() {}

ContentComponent* GuiService::getContentComponent()
{
    jassert (factory != nullptr);
    if (! content)
    {
        const auto uitype = context().settings().getMainContentType();
        content.reset();
        content = factory->createMainContent (uitype);
        content->setSize (760, 480);
    }

    return content.get();
}

int GuiService::getNumPluginWindows() const
{
    return (nullptr != windowManager) ? windowManager->getNumPluginWindows()
                                      : 0;
}

PluginWindow* GuiService::getPluginWindow (const int window) const
{
    return (nullptr != windowManager) ? windowManager->getPluginWindow (window)
                                      : nullptr;
}

PluginWindow* GuiService::getPluginWindow (const Node& node) const
{
    for (int i = 0; i < getNumPluginWindows(); ++i)
        if (auto* const window = getPluginWindow (i))
            if (window->getNode() == node)
                return window;
    return nullptr;
}

void GuiService::showPluginWindowsFor (const Node& node, const bool recursive, const bool force, const bool focus)
{
    if (! node.isGraph())
    {
        if (force || (bool) node.getProperty ("windowVisible", false))
            presentPluginWindow (node, force);
        return;
    }

    if (node.isGraph() && recursive)
        for (int i = 0; i < node.getNumNodes(); ++i)
            showPluginWindowsFor (node.getNode (i), recursive, force, focus);
}

void GuiService::presentPluginWindow (const Node& node, const bool focus)
{
    if (! windowManager)
        return;

    if (node.isIONode())
    {
        DBG ("[element] not showing pugin window for: " << node.getName());
        return;
    }

    auto* window = windowManager->getPluginWindowFor (node);
    if (! window)
        window = windowManager->createPluginWindowFor (node);

    if (window != nullptr)
    {
        window->setName (String());

        if (getRunMode() == RunMode::Plugin)
        {
            // This makes plugin window handling more like the standalone
            // we don't want to modify the existing standalone behavior
            window->setAlwaysOnTop (true);
        }

        window->setVisible (true);
        window->toFront (focus);
    }
}

bool GuiService::haveActiveWindows() const
{
    if (mainWindow && mainWindow->isActiveWindow())
        return true;
    for (int i = 0; i < getNumPluginWindows(); ++i)
        if (getPluginWindow (i)->isActiveWindow())
            return true;
    return false;
}

void GuiService::run()
{
    auto& settings = context().settings();
    PropertiesFile* const pf = settings.getUserSettings();

    mainWindow.reset (new MainWindow (world));
    mainWindow->windowTitleFunction = factory->getMainWindowTitler();
    if (auto menu = factory->createMainMenuBarModel())
    {
        mainWindow->setMainMenuModel (std::move (menu));
    }

    mainWindow->setContentNonOwned (getContentComponent(), true);
    mainWindow->centreWithSize (content->getWidth(), content->getHeight());
    mainWindow->restoreWindowStateFromString (pf->getValue ("mainWindowState"));
    mainWindow->addKeyListener (keys.get());
    mainWindow->addKeyListener (commands().getKeyMappings());
    getContentComponent()->restoreState (pf);
    mainWindow->addToDesktop();

    Desktop::getInstance().setGlobalScaleFactor (
        context().settings().getDesktopScale());

    if (pf->getBoolValue ("mainWindowVisible", true))
    {
        mainWindow->setVisible (true);
        if (pf->getBoolValue ("mainWindowFullScreen", false))
            mainWindow->setFullScreen (true);
    }
    else
    {
        mainWindow->setVisible (false);
        mainWindow->removeFromDesktop();
    }

    sibling<SessionService>()->resetChanges();
    refreshSystemTray();
    stabilizeViews();
}

SessionRef GuiService::session()
{
    if (! sessionRef)
        sessionRef = world.session();
    return sessionRef;
}

ApplicationCommandTarget* GuiService::getNextCommandTarget()
{
    return nullptr;
}

void GuiService::getAllCommands (Array<CommandID>& ids)
{
    ids.addArray ({
        Commands::sessionNew,
        Commands::sessionSave,
        Commands::sessionSaveAs,
        Commands::sessionOpen,
        Commands::sessionAddGraph,
        Commands::sessionDuplicateGraph,
        Commands::sessionDeleteGraph,
        Commands::sessionInsertPlugin,

        Commands::importGraph,
        Commands::exportGraph,
        Commands::panic,
        Commands::checkNewerVersion,
        Commands::transportPlay,
        Commands::graphNew,
        Commands::graphOpen,
        Commands::graphSave,
        Commands::graphSaveAs,
        Commands::importSession,
        Commands::recentsClear,
    });

    ids.addArray ({ Commands::copy, Commands::paste, Commands::undo, Commands::redo });
    ids.addArray ({ Commands::showSessionConfig,
                    Commands::showGraphMixer,
                    Commands::showConsole,
                    Commands::toggleChannelStrip,
                    Commands::showAbout,
                    Commands::showPluginManager,
                    Commands::showPreferences,
                    Commands::showGraphConfig,
                    Commands::showPatchBay,
                    Commands::showGraphEditor,
                    Commands::showLastContentView,
                    Commands::toggleVirtualKeyboard,
                    Commands::toggleMeterBridge,
                    Commands::rotateContentView,
                    Commands::showAllPluginWindows,
                    Commands::hideAllPluginWindows,
                    Commands::showKeymapEditor,
                    Commands::showControllers,
                    Commands::toggleUserInterface });

    ids.add (Commands::quit);
    if (content)
        content->getAllCommands (ids);
}

void GuiService::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    using Info = juce::ApplicationCommandInfo;
    auto& undo = impl->undo;
    auto& recents = impl->recents;

    switch (commandID)
    {
        case Commands::exportAudio:
            result.setInfo ("Export Audio", "Export to an audio file", "Session", 0);
            break;
        case Commands::exportMidi:
            result.setInfo ("Exort MIDI", "Export to a MIDI file", "Session", 0);
            break;
        case Commands::importGraph:
            result.setInfo ("Import graph", "Import a graph into current session", "Session", 0);
            break;
        case Commands::exportGraph:
            result.setInfo ("Export current graph", "Export the current graph to file", "Session", 0);
            break;
        case Commands::panic:
            result.addDefaultKeypress ('p', ModifierKeys::altModifier | ModifierKeys::commandModifier);
            result.setInfo ("Panic!", "Sends all notes off to the engine", "Engine", 0);
            break;

        // MARK: Session Commands
        case Commands::sessionClose:
            result.setInfo ("Close Session", "Close the current session", "Session", 0);
            break;
        case Commands::sessionNew:
            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Session", "Create a new session", "Session", 0);
            break;
        case Commands::sessionOpen:
            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Session", "Open an existing session", "Session", 0);
            break;
        case Commands::sessionSave:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            result.setInfo ("Save Session", "Save the current session", "Session", 0);
            break;
        case Commands::sessionSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session As", "Save the current session with a new name", "Session", 0);
            break;
        case Commands::sessionAddGraph:
            result.addDefaultKeypress ('n', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
            result.setInfo ("Add graph", "Add a new graph to the session", "Session", 0);
            break;
        case Commands::sessionDuplicateGraph:
            result.addDefaultKeypress ('d', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
            result.setInfo ("Duplicate current graph", "Duplicates the currently active graph", "Session", 0);
            break;
        case Commands::sessionDeleteGraph:
            result.addDefaultKeypress (KeyPress::backspaceKey, ModifierKeys::commandModifier);
            result.setInfo ("Delete current graph", "Deletes the current graph", "Session", 0);
            break;
        case Commands::sessionInsertPlugin:
            result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            result.setInfo ("Insert plugin", "Add a plugin in the current graph", "Session", Info::isDisabled);
            break;
        case Commands::showSessionConfig: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "SessionSettings")
                flags |= Info::isTicked;
            result.setInfo ("Session Settings", "Session Settings", "Session", flags);
        }
        break;

        // MARK: Show Commands
        case Commands::showPreferences:
            result.setInfo ("Show Preferences", "Element Preferences", "Application", 0);
            result.addDefaultKeypress (',', ModifierKeys::commandModifier);
            break;
        case Commands::showAbout:
            result.setInfo ("Show About", "About this program", "Application", 0);
            break;
        case Commands::showLegacyView:
            result.setInfo ("Legacy View", "Shows the legacy Beat Thang Virtual GUI", "UI", 0);
            break;
        case Commands::showPluginManager:
            result.setInfo ("Plugin Manager", "Element Plugin Management", "Application", 0);
            break;

        case Commands::showLastContentView:
            result.setInfo ("Last View", "Shows the last content view", "UI", 0);
            break;

        case Commands::showGraphConfig: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "GraphSettings")
                flags |= Info::isTicked;
            result.setInfo ("Graph Settings", "Graph Settings", "Session", flags);
        }
        break;

        case Commands::showPatchBay: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "PatchBay")
                flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F1Key, 0);
            result.setInfo ("Patch Bay", "Show the patch bay", "Session", flags);
        }
        break;

        case Commands::showGraphEditor: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "GraphEditor")
                flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F2Key, 0);
            result.setInfo ("Graph Editor", "Show the graph editor", "UI", flags);
        }
        break;

        case Commands::showGraphMixer: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->showAccessoryView() && content->getAccessoryViewName() == EL_VIEW_GRAPH_MIXER)
            {
                flags |= Info::isTicked;
            }
            result.setInfo ("Graph Mixer", "Show/hide the graph mixer", "UI", flags);
        }
        break;

        case Commands::showConsole: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->showAccessoryView() && content->getAccessoryViewName() == EL_VIEW_CONSOLE)
            {
                flags |= Info::isTicked;
            }
            result.setInfo ("Console", "Show the scripting console", "UI", flags);
        }
        break;

        case Commands::showControllers: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "ControllersView")
                flags |= Info::isTicked;
            result.setInfo ("Controller Devices", "Show the session's controllers", "Session", flags);
        }
        break;

        case Commands::toggleUserInterface:
            result.setInfo ("Show/Hide UI", "Toggles visibility of the user interface", "UI", 0);
            break;

        case Commands::toggleChannelStrip: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isNodeChannelStripVisible())
                flags |= Info::isTicked;
            result.setInfo ("Channel Strip", "Toggles the global channel strip", "UI", flags);
        }
        break;

        case Commands::toggleVirtualKeyboard: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isVirtualKeyboardVisible())
                flags |= Info::isTicked;
            result.setInfo ("Virtual Keyboard", "Toggle the virtual keyboard", "UI", flags);
        }
        break;

        case Commands::toggleMeterBridge: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isMeterBridgeVisible())
                flags |= Info::isTicked;
            result.setInfo ("MeterBridge", "Toggle the Meter Bridge", "UI", flags);
        }
        break;

        case Commands::rotateContentView:
            result.addDefaultKeypress ('r', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            result.setInfo ("Rotate View", "Show the graph editor", "Session", 0);
            break;

        case Commands::showAllPluginWindows:
            result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Show all plugin windows", "Show all plugins for the current graph.", "Session", 0);
            break;
        case Commands::hideAllPluginWindows:
            result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            result.setInfo ("Hide all plugin windows", "Hides all plugins on the current graph.", "Session", 0);
            break;
        case Commands::showKeymapEditor:
            // result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Keymap Editor", "Show the keyboard shortcuts and edit them.", "UI", 0);
            break;

        case Commands::checkNewerVersion:
            result.setInfo ("Check For Updates", "Check newer version", "Application", 0);
            break;

        case Commands::quit:
            result.setInfo ("Quit", "Quit the app", "Application", 0);
            result.addDefaultKeypress ('q', ModifierKeys::commandModifier);
            break;

        case Commands::undo: {
            int flags = undo.canUndo() ? 0 : Info::isDisabled;
            result.setInfo ("Undo", "Undo the last operation", "Application", flags);
            result.addDefaultKeypress ('z', ModifierKeys::commandModifier);
        }
        break;
        case Commands::redo: {
            bool canRedo = undo.canRedo();
            int flags = canRedo ? 0 : Info::isDisabled;
            result.setInfo ("Redo", "Redo the last operation", "Application", flags);
            result.addDefaultKeypress ('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        }
        break;

        case Commands::cut:
            result.setInfo ("Cut", "Cut", "Application", 0);
            break;
        case Commands::copy:
            result.addDefaultKeypress ('c', ModifierKeys::commandModifier);
            result.setInfo ("Copy", "Copy", "Application", Info::isDisabled);
            break;
        case Commands::paste:
            result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            result.setInfo ("Paste", "Paste", "Application", Info::isDisabled);
            break;
        case Commands::selectAll:
            result.setInfo ("Select All", "Select all", "Application", 0);
            break;

        case Commands::transportRewind:
            result.setInfo ("Rewind", "Transport Rewind", "Engine", 0);
            result.addDefaultKeypress ('j', 0);
            break;
        case Commands::transportForward:
            result.setInfo ("Forward", "Transport Fast Forward", "Engine", 0);
            result.addDefaultKeypress ('l', 0);
            break;
        case Commands::transportPlay:
            result.setInfo ("Play", "Transport Play", "Engine", 0);
            result.addDefaultKeypress (KeyPress::spaceKey, 0);
            break;
        case Commands::transportRecord:
            result.setInfo ("Record", "Transport Record", "Engine", 0);
            break;
        case Commands::transportSeekZero:
            result.setInfo ("Seek Start", "Seek to Beginning", "Engine", 0);
            break;
        case Commands::transportStop:
            result.setInfo ("Stop", "Transport Stop", "Engine", 0);
            break;

        case Commands::recentsClear:
            result.setInfo ("Clear Recent Files", "Clears the recently opened files list", "Application", 0);
            result.setActive (recents.getNumFiles() > 0);
            break;
    }

    if (content)
        content->getCommandInfo (commandID, result);
}

bool GuiService::perform (const InvocationInfo& info)
{
    if (content && content->perform (info))
    {
        if (mainWindow)
            mainWindow->refreshMenu();
        return true;
    }

    bool result = true;
    switch (info.commandID)
    {
        case Commands::showAbout:
            toggleAboutScreen();
            break;
        case Commands::showPreferences:
            runDialog ("preferences");
            break;
        case Commands::showAllPluginWindows: {
            if (auto s = context().session())
                showPluginWindowsFor (s->getActiveGraph(), true, true);
            break;
        }

        case Commands::hideAllPluginWindows: {
            closeAllPluginWindows (false);
            break;
        }

        case Commands::toggleUserInterface: {
            auto session = context().session();
            if (auto* const window = mainWindow.get())
            {
                if (window->isOnDesktop())
                {
                    window->removeFromDesktop();
                    closeAllPluginWindows (true);
                }
                else
                {
                    window->addToDesktop();
                    window->toFront (true);
                    if (session)
                        showPluginWindowsFor (session->getActiveGraph(), true, false);
                }
            }
        }
        break;

        case Commands::quit:
            JUCEApplication::getInstance()->systemRequestedQuit();
            break;

        default:
            result = false;
            break;
    }

    if (result && mainWindow)
    {
        mainWindow->refreshMenu();
    }

    if (result)
        return result;
    auto& undo = impl->undo;
    bool res = true;

    switch (info.commandID)
    {
        case Commands::undo: {
            if (undo.canUndo())
                undo.undo();
            if (auto* cc = getContentComponent())
                cc->stabilizeViews();
            refreshMainMenu();
        }
        break;

        case Commands::redo: {
            if (undo.canRedo())
                undo.redo();
            if (auto* cc = getContentComponent())
                cc->stabilizeViews();
            refreshMainMenu();
        }
        break;

        case Commands::sessionOpen: {
            FileChooser chooser ("Open Session", impl->lastSavedFile, "*.els", true, false);
            if (chooser.browseForFileToOpen())
            {
                sibling<SessionService>()->openFile (chooser.getResult());
                impl->recents.addFile (chooser.getResult());
            }
        }
        break;

        case Commands::sessionNew:
            sibling<SessionService>()->newSession();
            break;
        case Commands::sessionSave:
            sibling<SessionService>()->saveSession (false);
            break;
        case Commands::sessionSaveAs:
            sibling<SessionService>()->saveSession (true);
            break;
        case Commands::sessionClose:
            sibling<SessionService>()->closeSession();
            break;
        case Commands::sessionAddGraph:
            sibling<EngineService>()->addGraph();
            break;
        case Commands::sessionDuplicateGraph:
            sibling<EngineService>()->duplicateGraph();
            break;
        case Commands::sessionDeleteGraph:
            sibling<EngineService>()->removeGraph();
            break;

        case Commands::transportPlay:
            context().audio()->togglePlayPause();
            break;

        case Commands::importGraph: {
            FileChooser chooser ("Import Graph", impl->lastExportedGraph, "*.elg");
            if (chooser.browseForFileToOpen())
                sibling<SessionService>()->importGraph (chooser.getResult());
        }
        break;

        case Commands::exportGraph: {
            auto session = context().session();
            auto node = session->getCurrentGraph();
            node.savePluginState();

            if (! impl->lastExportedGraph.isDirectory())
                impl->lastExportedGraph = impl->lastExportedGraph.getParentDirectory();
            if (impl->lastExportedGraph.isDirectory())
            {
                impl->lastExportedGraph = impl->lastExportedGraph.getChildFile (node.getName()).withFileExtension ("elg");
                impl->lastExportedGraph = impl->lastExportedGraph.getNonexistentSibling();
            }

            {
                FileChooser chooser (TRANS ("Export Graph"), impl->lastExportedGraph, "*.elg");
                if (chooser.browseForFileToSave (true))
                    sibling<SessionService>()->exportGraph (node, chooser.getResult());
                if (auto* gui = sibling<GuiService>())
                    gui->stabilizeContent();
            }
        }
        break;

        case Commands::panic: {
            auto e = context().audio();
            for (int c = 1; c <= 16; ++c)
            {
                auto msg = MidiMessage::allNotesOff (c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
                msg = MidiMessage::allSoundOff (c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
            }
        }
        break;

        case Commands::checkNewerVersion:
            checkUpdates();
            break;

        case Commands::recentsClear: {
            impl->recents.clear();
            refreshMainMenu();
        }
        break;

        default:
            res = false;
            break;
    }

    return res;
}

void GuiService::stabilizeContent()
{
    if (auto* cc = content.get())
        cc->stabilize();
    refreshMainMenu();
    refreshSystemTray();
    if (mainWindow)
        mainWindow->refreshName();
    sigRefreshed();
}

void GuiService::stabilizeViews()
{
    if (auto* cc = content.get())
    {
        const auto shouldBeEnabled = true;
        if (cc->isEnabled() != shouldBeEnabled)
            cc->setEnabled (shouldBeEnabled);
        cc->stabilizeViews();
    }
}

void GuiService::refreshSystemTray()
{
    // stabilize systray
    auto& settings = context().settings();
    SystemTray::setEnabled (settings.isSystrayEnabled());
}

void GuiService::setMainWindowTitler (std::function<juce::String()> f)
{
    if (mainWindow)
    {
        mainWindow->windowTitleFunction = f;
        mainWindow->refreshName();
    }
}

void GuiService::refreshMainMenu()
{
    if (auto* win = mainWindow.get())
        win->refreshMenu();
}

void GuiService::toggleAboutScreen()
{
    if (! about)
    {
        about.reset (new AboutDialog (*this));
        if (appInfo.title.isNotEmpty())
        {
            if (auto c = dynamic_cast<AboutComponent*> (about->getContentComponent()))
            {
                c->setAboutInfo (appInfo);
                about->setName (TRANS ("About ") + appInfo.title);
            }
        }
    }

    jassert (about);

    if (about->isOnDesktop())
    {
        about->removeFromDesktop();
        about->setVisible (false);
    }
    else
    {
        about->addToDesktop();
        about->centreWithSize (about->getWidth(), about->getHeight());
        about->setVisible (true);
        about->toFront (true);
        if (getRunMode() == RunMode::Plugin)
            about->setAlwaysOnTop (true);
    }
}

KeyListener* GuiService::getKeyListener() const { return keys.get(); }

bool GuiService::handleMessage (const AppMessage& msg)
{
    OwnedArray<UndoableAction> actions;
    msg.createActions (services(), actions);
    if (! actions.isEmpty())
    {
        auto& undo = impl->undo;
        undo.beginNewTransaction();
        for (auto* action : actions)
            undo.perform (action);
        actions.clearQuick (false);
        stabilizeViews();
        return true;
    }

    if (nullptr != dynamic_cast<const ReloadMainContentMessage*> (&msg))
    {
        auto& settings = context().settings();
        PropertiesFile* const pf = settings.getUserSettings();

        if (mainWindow && pf != nullptr)
        {
            const auto ws = mainWindow->getWindowStateAsString();
            clearContentComponent();
            mainWindow->setContentNonOwned (getContentComponent(), true);
            mainWindow->restoreWindowStateFromString (ws);
            content->restoreState (pf);
            stabilizeContent();
            refreshMainMenu();
            refreshSystemTray();
        }

        return true;
    }
    else if (auto m = dynamic_cast<const PresentViewMessage*> (&msg))
    {
        if (m->create && content != nullptr)
            if (auto* v = m->create())
                content->setMainView (v);

        return true;
    }

    return false;
}

GuiService::RecentFiles& GuiService::recentFiles() { return impl->recents; }

void GuiService::shutdown()
{
    if (updates->launchRequested())
        updates->launchDetached();
}

void GuiService::changeListenerCallback (ChangeBroadcaster* broadcaster)
{
    if (broadcaster == &context().devices())
        if (auto* win = mainWindow.get())
            win->refreshMenu();
}

void GuiService::clearContentComponent()
{
    if (about)
    {
        about->setVisible (false);
        about->removeFromDesktop();
        about = nullptr;
    }

    if (mainWindow)
    {
        mainWindow->clearContentComponent();
    }

    content.reset();
}

} // namespace element
