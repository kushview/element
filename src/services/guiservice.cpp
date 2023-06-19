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

#include <element/ui/windowdecorator.hpp>

#include <element/audioengine.hpp>
#include <element/services.hpp>

#include <element/services/guiservice.hpp>
#include "services/sessionservice.hpp"
#include "services/engineservice.hpp"

#include "gui/views/VirtualKeyboardView.h"

#include "gui/AboutComponent.h"
#include "gui/ContentComponent.h"
#include "gui/GuiCommon.h"
#include "gui/LookAndFeel.h"
#include "gui/MainWindow.h"
#include "gui/PluginWindow.h"
#include "gui/PreferencesComponent.h"
#include "gui/SystemTray.h"
#include "gui/WindowManager.h"
#include "gui/StandardContentComponent.h"
#include "gui/capslock.hpp"

#include "session/commandmanager.hpp"

#include "version.hpp"

namespace element {

//=============================================================================
class DefaultContentFactory : public WindowDecorator
{
public:
    DefaultContentFactory (Context& ctx)
        : context (ctx) {}
    ~DefaultContentFactory() {}

    std::unique_ptr<ContentComponent> createMainContent (const String& type) override
    {
        if (type == "standard")
            return std::make_unique<StandardContentComponent> (context.getServices());
        if (type == "compact")
            return std::make_unique<StandardContentComponent> (context.getServices());

        return std::make_unique<StandardContentComponent> (context.getServices());
    }

private:
    Context& context;
};

//=============================================================================
struct GlobalLookAndFeel
{
    GlobalLookAndFeel() { LookAndFeel::setDefaultLookAndFeel (&look); }
    ~GlobalLookAndFeel() { LookAndFeel::setDefaultLookAndFeel (nullptr); }
    element::LookAndFeel look;
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

        const auto gp = KeyPress::createFromDescription (g.getProperty (Tags::keyMap));
        if (gp.isValid() && key == gp)
        {
            owner.closeAllPluginWindows (true);
            auto graphs = session->getValueTree().getChildWithName (Tags::graphs);
            graphs.setProperty (Tags::active, graphs.indexOf (g.getValueTree()), 0);
            owner.findSibling<EngineService>()->setRootNode (g);
            owner.showPluginWindowsFor (g, true, false, false);
            return true;
        }

        return false;
    }

    GuiService& owner;
};

static std::unique_ptr<GlobalLookAndFeel> sGlobalLookAndFeel;
static Array<GuiService*> sGuiControllerInstances;

//=============================================================================
void GuiService::ForegroundCheck::timerCallback()
{
    static bool sIsForeground = true;
    auto foreground = Process::isForegroundProcess();
    if (sIsForeground == foreground)
        return;

    if (! ui.getSettings().hidePluginWindowsWhenFocusLost())
        return;

    auto session = ui.getWorld().getSession();
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
void GuiService::setMainWindowDecorator (std::unique_ptr<WindowDecorator> newDecorator)
{
    clearContentComponent();
    if (mainWindow)
        mainWindow->windowTitleFunction = nullptr;
    if (factory)
        factory.reset();
    factory = std::move (newDecorator);
    if (! factory)
        factory = std::make_unique<DefaultContentFactory> (getWorld());
}

//=============================================================================
GuiService::GuiService (Context& w, ServiceManager& a)
    : Service(),
      controller (a),
      world (w),
      windowManager (nullptr),
      mainWindow (nullptr),
      foregroundCheck (*this)
{
    keys.reset (new KeyPressManager (*this));
    if (sGuiControllerInstances.size() <= 0)
        sGlobalLookAndFeel.reset (new GlobalLookAndFeel());
    sGuiControllerInstances.add (this);
    windowManager.reset (new WindowManager (*this));
    factory = std::make_unique<DefaultContentFactory> (w);
}

GuiService::~GuiService()
{
    sGuiControllerInstances.removeFirstMatchingValue (this);
    if (sGuiControllerInstances.size() <= 0)
        sGlobalLookAndFeel = nullptr;
}

element::LookAndFeel& GuiService::getLookAndFeel()
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
    getWorld().getDeviceManager().addChangeListener (this);
    Service::activate();
}

void GuiService::deactivate()
{
    getWorld().getDeviceManager().removeChangeListener (this);
    nodeSelected.disconnect_all_slots();

    auto& settings = getSettings();
    saveProperties (settings.getUserSettings());

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

    Service::deactivate();
}

void GuiService::closeAllWindows()
{
    if (! windowManager)
        return;
    windowManager->closeAll();
}

CommandManager& GuiService::commander() { return world.getCommandManager(); }

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

void GuiService::setAppInfo (const AppInfo& info)
{
    appInfo = info;
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
        const auto uitype = getWorld().getSettings().getMainContentType();
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
    auto& settings = getWorld().getSettings();
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
    mainWindow->addKeyListener (commander().getKeyMappings());
    getContentComponent()->restoreState (pf);
    mainWindow->addToDesktop();

    Desktop::getInstance().setGlobalScaleFactor (
        getWorld().getSettings().getDesktopScale());

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

    findSibling<SessionService>()->resetChanges();
    refreshSystemTray();
    stabilizeViews();
}

SessionRef GuiService::session()
{
    if (! sessionRef)
        sessionRef = world.getSession();
    return sessionRef;
}

ApplicationCommandTarget* GuiService::getNextCommandTarget()
{
    return nullptr;
}

void GuiService::getAllCommands (Array<CommandID>& commands)
{
    commands.addArray ({
        Commands::showSessionConfig,
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
        Commands::showControllerDevices,
        Commands::toggleUserInterface });

    commands.add (Commands::quit);
    if (content)
        content->getAllCommands (commands);
}

void GuiService::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    typedef ApplicationCommandInfo Info;
    auto& app = getServices();

    switch (commandID)
    {
        case Commands::exportAudio:
            result.setInfo ("Export Audio", "Export to an audio file", Commands::Categories::Session, 0);
            break;
        case Commands::exportMidi:
            result.setInfo ("Exort MIDI", "Export to a MIDI file", Commands::Categories::Session, 0);
            break;
        case Commands::importGraph:
            result.setInfo ("Import graph", "Import a graph into current session", Commands::Categories::Session, 0);
            break;
        case Commands::exportGraph:
            result.setInfo ("Export current graph", "Export the current graph to file", Commands::Categories::Session, 0);
            break;
        case Commands::panic:
            result.addDefaultKeypress ('p', ModifierKeys::altModifier | ModifierKeys::commandModifier);
            result.setInfo ("Panic!", "Sends all notes off to the engine", "Engine", 0);
            break;

        // MARK: Session Commands
        case Commands::sessionClose:
            result.setInfo ("Close Session", "Close the current session", Commands::Categories::Session, 0);
            break;
        case Commands::sessionNew:
            result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            result.setInfo ("New Session", "Create a new session", Commands::Categories::Session, 0);
            break;
        case Commands::sessionOpen:
            result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            result.setInfo ("Open Session", "Open an existing session", Commands::Categories::Session, 0);
            break;
        case Commands::sessionSave:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            result.setInfo ("Save Session", "Save the current session", Commands::Categories::Session, 0);
            break;
        case Commands::sessionSaveAs:
            result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Save Session As", "Save the current session with a new name", Commands::Categories::Session, 0);
            break;
        case Commands::sessionAddGraph:
            result.addDefaultKeypress ('n', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
            result.setInfo ("Add graph", "Add a new graph to the session", Commands::Categories::Session, 0);
            break;
        case Commands::sessionDuplicateGraph:
            result.addDefaultKeypress ('d', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
            result.setInfo ("Duplicate current graph", "Duplicates the currently active graph", Commands::Categories::Session, 0);
            break;
        case Commands::sessionDeleteGraph:
            result.addDefaultKeypress (KeyPress::backspaceKey, ModifierKeys::commandModifier);
            result.setInfo ("Delete current graph", "Deletes the current graph", Commands::Categories::Session, 0);
            break;
        case Commands::sessionInsertPlugin:
            result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            result.setInfo ("Insert plugin", "Add a plugin in the current graph", Commands::Categories::Session, Info::isDisabled);
            break;
        case Commands::showSessionConfig: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "SessionSettings")
                flags |= Info::isTicked;
            result.setInfo ("Session Settings", "Session Settings", Commands::Categories::Session, flags);
        }
        break;

        // MARK: Media Commands
        case Commands::mediaNew:
            result.setInfo ("New Media", "Close the current media", Commands::Categories::Session, 0);
            break;
        case Commands::mediaClose:
            result.setInfo ("Close Media", "Close the current media", Commands::Categories::Session, 0);
            break;
        case Commands::mediaOpen:
            result.setInfo ("Open Media", "Opens a type of supported media", Commands::Categories::Session, 0);
            break;
        case Commands::mediaSave:
            result.setInfo ("Save Media", "Saves the currently viewed object", Commands::Categories::Session, 0);
            break;

        case Commands::mediaSaveAs:
            result.setInfo ("Save Media As", "Saves the current object with another name", Commands::Categories::Session, 0);
            break;

        // MARK: Show Commands
        case Commands::showPreferences:
            result.setInfo ("Show Preferences", "Element Preferences", Commands::Categories::Application, 0);
            result.addDefaultKeypress (',', ModifierKeys::commandModifier);
            break;
        case Commands::showAbout:
            result.setInfo ("Show About", "About this program", Commands::Categories::Application, 0);
            break;
        case Commands::showLegacyView:
            result.setInfo ("Legacy View", "Shows the legacy Beat Thang Virtual GUI", Commands::Categories::UserInterface, 0);
            break;
        case Commands::showPluginManager:
            result.setInfo ("Plugin Manager", "Element Plugin Management", Commands::Categories::Application, 0);
            break;

        case Commands::showLastContentView:
            result.setInfo ("Last View", "Shows the last content view", Commands::Categories::UserInterface, 0);
            break;

        case Commands::showGraphConfig: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "GraphSettings")
                flags |= Info::isTicked;
            result.setInfo ("Graph Settings", "Graph Settings", Commands::Categories::Session, flags);
        }
        break;

        case Commands::showPatchBay: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "PatchBay")
                flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F1Key, 0);
            result.setInfo ("Patch Bay", "Show the patch bay", Commands::Categories::Session, flags);
        }
        break;

        case Commands::showGraphEditor: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "GraphEditor")
                flags |= Info::isTicked;
            result.addDefaultKeypress (KeyPress::F2Key, 0);
            result.setInfo ("Graph Editor", "Show the graph editor", Commands::Categories::UserInterface, flags);
        }
        break;

        case Commands::showGraphMixer: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->showAccessoryView() && content->getAccessoryViewName() == EL_VIEW_GRAPH_MIXER)
            {
                flags |= Info::isTicked;
            }
            result.setInfo ("Graph Mixer", "Show/hide the graph mixer", Commands::Categories::UserInterface, flags);
        }
        break;

        case Commands::showConsole: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->showAccessoryView() && content->getAccessoryViewName() == EL_VIEW_CONSOLE)
            {
                flags |= Info::isTicked;
            }
            result.setInfo ("Console", "Show the scripting console", Commands::Categories::UserInterface, flags);
        }
        break;

        case Commands::showControllerDevices: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->getMainViewName() == "ControllerDevicesView")
                flags |= Info::isTicked;
            result.setInfo ("Controller Devices", "Show the session's controllers", Commands::Categories::Session, flags);
        }
        break;

        case Commands::toggleUserInterface:
            result.setInfo ("Show/Hide UI", "Toggles visibility of the user interface", Commands::Categories::UserInterface, 0);
            break;

        case Commands::toggleChannelStrip: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isNodeChannelStripVisible())
                flags |= Info::isTicked;
            result.setInfo ("Channel Strip", "Toggles the global channel strip", Commands::Categories::UserInterface, flags);
        }
        break;

        case Commands::toggleVirtualKeyboard: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isVirtualKeyboardVisible())
                flags |= Info::isTicked;
            result.setInfo ("Virtual Keyboard", "Toggle the virtual keyboard", Commands::Categories::UserInterface, flags);
        }
        break;

        case Commands::toggleMeterBridge: {
            int flags = (content != nullptr) ? 0 : Info::isDisabled;
            if (content && content->isMeterBridgeVisible())
                flags |= Info::isTicked;
            result.setInfo ("MeterBridge", "Toggle the Meter Bridge", Commands::Categories::UserInterface, flags);
        }
        break;

        case Commands::rotateContentView:
            result.addDefaultKeypress ('r', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            result.setInfo ("Rotate View", "Show the graph editor", Commands::Categories::Session, 0);
            break;

        case Commands::showAllPluginWindows:
            result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Show all plugin windows", "Show all plugins for the current graph.", Commands::Categories::Session, 0);
            break;
        case Commands::hideAllPluginWindows:
            result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            result.setInfo ("Hide all plugin windows", "Hides all plugins on the current graph.", Commands::Categories::Session, 0);
            break;
        case Commands::showKeymapEditor:
            // result.addDefaultKeypress ('w', ModifierKeys::commandModifier | ModifierKeys::altModifier | ModifierKeys::shiftModifier);
            result.setInfo ("Keymap Editor", "Show the keyboard shortcuts and edit them.", Commands::Categories::UserInterface, 0);
            break;

        case Commands::checkNewerVersion:
            result.setInfo ("Check For Updates", "Check newer version", Commands::Categories::Application, 0);
            break;

        case Commands::signIn:
            result.setInfo ("Sign In", "Saves the current object with another name", Commands::Categories::Application, 0);
            break;
        case Commands::signOut:
            result.setInfo ("Sign Out", "Saves the current object with another name", Commands::Categories::Application, 0);
            break;

        case Commands::quit:
            result.setInfo ("Quit", "Quit the app", Commands::Categories::Application, 0);
            result.addDefaultKeypress ('q', ModifierKeys::commandModifier);
            break;

        case Commands::undo: {
            int flags = getServices().getUndoManager().canUndo() ? 0 : Info::isDisabled;
            result.setInfo ("Undo", "Undo the last operation", Commands::Categories::Application, flags);
            result.addDefaultKeypress ('z', ModifierKeys::commandModifier);
        }
        break;
        case Commands::redo: {
            bool canRedo = getServices().getUndoManager().canRedo();
            int flags = canRedo ? 0 : Info::isDisabled;
            result.setInfo ("Redo", "Redo the last operation", Commands::Categories::Application, flags);
            result.addDefaultKeypress ('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        }
        break;

        case Commands::cut:
            result.setInfo ("Cut", "Cut", Commands::Categories::Application, 0);
            break;
        case Commands::copy:
            result.addDefaultKeypress ('c', ModifierKeys::commandModifier);
            result.setInfo ("Copy", "Copy", Commands::Categories::Application, Info::isDisabled);
            break;
        case Commands::paste:
            result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            result.setInfo ("Paste", "Paste", Commands::Categories::Application, Info::isDisabled);
            break;
        case Commands::selectAll:
            result.setInfo ("Select All", "Select all", Commands::Categories::Application, 0);
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
            result.setInfo ("Clear Recent Files", "Clears the recently opened files list", Commands::Categories::Application, 0);
            result.setActive (app.getRecentlyOpenedFilesList().getNumFiles() > 0);
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
            if (auto s = getWorld().getSession())
                showPluginWindowsFor (s->getActiveGraph(), true, true);
            break;
        }

        case Commands::hideAllPluginWindows: {
            closeAllPluginWindows (false);
            break;
        }

        case Commands::toggleUserInterface: {
            auto session = getWorld().getSession();
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

    return result;
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
    auto& settings = getWorld().getSettings();
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
                c->setAppInfo (appInfo);
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
    if (nullptr != dynamic_cast<const ReloadMainContentMessage*> (&msg))
    {
        auto& settings = getWorld().getSettings();
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

void GuiService::changeListenerCallback (ChangeBroadcaster* broadcaster)
{
    if (broadcaster == &getWorld().getDeviceManager())
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
