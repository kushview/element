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

#include "controllers/DevicesController.h"
#include "controllers/MappingController.h"
#include "controllers/SessionController.h"
#include "controllers/GraphController.h"
#include "controllers/GuiController.h"
#include "gui/ContentComponent.h"
#include "gui/MainWindow.h"
#include "gui/ViewHelpers.h"
#include "gui/PluginWindow.h"
#include "gui/Workspace.h"
#include "gui/WorkspacesContentComponent.h"
#include "engine/AudioEngine.h"
#include "session/Session.h"
#include "session/CommandManager.h"
#include "session/Node.h"
#include "Commands.h"
#include "Globals.h"
#include "Settings.h"
#include "Utils.h"
#include "URLs.h"

#include "gui/MainMenu.h"

namespace Element {

MainMenu::MainMenu (MainWindow& parent, CommandManager& c)
    : owner (parent), world (parent.getWorld()), cmd (c) {}

MainMenu::~MainMenu()
{
#if JUCE_MAC
    MainMenu::setMacMainMenu (nullptr);
    macMenu = nullptr;
#else
    owner.setMenuBar (nullptr);
#endif
}

void MainMenu::setupMenu()
{
#if JUCE_MAC
    macMenu.reset (new PopupMenu());
    macMenu->addCommandItem (&cmd, Commands::showAbout, Util::appName ("About"));
    macMenu->addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates...");
    macMenu->addSeparator();
    macMenu->addCommandItem (&cmd, Commands::showPreferences, "Preferences...");
    MenuBarModel::setMacMainMenu (this, macMenu.get());
#else
    owner.setMenuBar (this);
#endif
}

StringArray MainMenu::getMenuBarNames()
{
    const char* const names[] = {
        "File",
        "Edit",
        "View",
        "Options",
        "Window",
#if JUCE_DEBUG
        "Debug",
#endif
        "Help",
        nullptr
    };

    return StringArray (names, MainMenu::NumMenus);
}

PopupMenu MainMenu::getMenuForIndex (int index, const String& name)
{
    ignoreUnused (index);
    PopupMenu menu;

    if (name == "File")
        buildFileMenu (menu);
    else if (name == "Edit")
        buildEditMenu (menu);
    else if (name == "View")
        buildViewMenu (menu);
    else if (name == "Window")
        buildWindowMenu (menu);
    else if (name == "Options")
        buildOptionsMenu (menu);
    else if (name == "Help")
        buildHelpMenu (menu);
#if JUCE_DEBUG
    else if (name == "Debug")
        buildDebugMenu (menu);
#endif

    return menu;
}

ContentComponent* MainMenu::getContentComponent()
{
    return dynamic_cast<ContentComponent*> (owner.getContentComponent());
}

void MainMenu::menuItemSelected (int index, int menu)
{
    auto session = world.getSession();
    auto engine = world.getAudioEngine();

    if (index == 6000 && menu == Help)
    {
        URL (EL_URL_MANUAL_HOME).launchInDefaultBrowser();
    }
    else if (index == 6500 && menu == Help)
    {
#ifdef EL_URL_API_LUA_EL
        URL (EL_URL_API_LUA_EL).launchInDefaultBrowser();
#endif
    }
    else if (index == 6501 && menu == Help)
    {
#ifdef EL_URL_API_LUA_KV
        URL (EL_URL_API_LUA_KV).launchInDefaultBrowser();
#endif
    }
    else if (index == 7000 && menu == Help)
    {
        URL (EL_URL_BUG_TRACKER).launchInDefaultBrowser();
    }
    else if (index == 2000 && menu == Window)
    {
        ViewHelpers::closePluginWindows (&owner, false);
    }

    if (menu == Options)
    {
        world.getSettings().performMenuResult (world, index);
        owner.refreshMenu();
    }

    if (menu == Window)
    {
        if (index >= 100000)
        {
            if (auto* const cc = getContentComponent())
                cc->handleWorkspaceMenuResult (index);
        }
    }

#if JUCE_DEBUG
    if (index == 1000)
    {
        DBG ("[EL] === SESSION DUMP ===");
        auto data = session->getValueTree().createCopy();
        Node::sanitizeProperties (data, true);
        DBG (data.toXmlString());
    }
    else if (index >= 1111 && index <= 1114)
    {
        const int program = index - 1111;
        auto msg = MidiMessage::programChange (1, program); // Program 10 in GUI equals Program 9 in MIDI
        msg.setTimeStamp (1.f + Time::getMillisecondCounterHiRes());
        engine->addMidiMessage (msg);
    }
    else if (index == 2222)
    {
        auto& app = owner.getAppController();
        DBG ("has changed: " << (int) app.findChild<SessionController>()->hasSessionChanged());
    }
    else if (index == 3333)
    {
        auto& app = owner.getAppController();
        if (auto* mapping = app.findChild<MappingController>())
            mapping->learn (true);
    }
    else if (index == 4444)
    {
        if (session)
            session->cleanOrphanControllerMaps();
        auto& app = owner.getAppController();
        if (auto* devices = app.findChild<DevicesController>())
            devices->refresh();
        if (auto* gui = app.findChild<GuiController>())
            gui->stabilizeContent();
    }
    else if (index == 5555)
    {
        if (auto* cc = getContentComponent())
            cc->setNodeChannelStripVisible (! cc->isNodeChannelStripVisible());
    }
    else if (index == 6666)
    {
        DBG ("no workspace debug window");
    }
    else if (index == 7777)
    {
        cmd.invokeDirectly (Commands::workspaceSave, true);
    }
    else if (index == 7778)
    {
        cmd.invokeDirectly (Commands::workspaceOpen, true);
    }
    else if (index == 8000)
    {
    }
    else if (index == 9000)
    {
        engine->addMidiMessage (MidiMessage::midiStart().withTimeStamp (1.f + Time::getMillisecondCounterHiRes()),
                                true);
    }
    else if (index == 9001)
    {
        engine->addMidiMessage (MidiMessage::midiStop().withTimeStamp (1.f + Time::getMillisecondCounterHiRes()),
                                true);
    }
    else if (index == 9002)
    {
        engine->addMidiMessage (MidiMessage::midiContinue().withTimeStamp (1.f + Time::getMillisecondCounterHiRes()),
                                true);
    }
#endif

    if (menu == File && index >= recentMenuOffset)
    {
        const int fileIdx = index - recentMenuOffset;
        class File f = owner.getAppController().getRecentlyOpenedFilesList().getFile (fileIdx);
#if defined(EL_PRO)
        owner.getAppController().findChild<SessionController>()->openFile (f);
#else
        owner.getAppController().findChild<GraphController>()->openGraph (f);
#endif
    }
}

void MainMenu::addRecentFiles (PopupMenu& menu)
{
    if (auto* cc = dynamic_cast<ContentComponent*> (owner.getContentComponent()))
    {
        PopupMenu recents;
        auto& app (cc->getAppController());
        auto& list (app.getRecentlyOpenedFilesList());
        if (list.getNumFiles() > 0)
        {
            list.createPopupMenuItems (recents, recentMenuOffset, false, true);
            recents.addSeparator();
        }

        recents.addCommandItem (&cmd, Commands::recentsClear, "Clear Recent Files");
        menu.addSubMenu ("Open Recent", recents);
        menu.addSeparator();
    }
}

void MainMenu::buildFileMenu (PopupMenu& menu)
{
#if defined(EL_SOLO)
    menu.addCommandItem (&cmd, Commands::graphNew, "New Graph");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::graphOpen, "Open Graph...");
    addRecentFiles (menu);
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::importSession, "Import from Session...");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::graphSave, "Save Graph");
    menu.addCommandItem (&cmd, Commands::graphSaveAs, "Save Graph As...");

#else
    menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::sessionOpen, "Open Session...");
    addRecentFiles (menu);
    menu.addCommandItem (&cmd, Commands::sessionSave, "Save Session");
    menu.addCommandItem (&cmd, Commands::sessionSaveAs, "Save Session As...");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::importGraph, "Import...");
    menu.addCommandItem (&cmd, Commands::exportGraph, "Export graph...");
#endif

#if ! JUCE_MAC
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates..");
    menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
    menu.addSeparator();
    menu.addCommandItem (&cmd, StandardApplicationCommandIDs::quit);
#endif
}

void MainMenu::buildEditMenu (PopupMenu& menu) { buildEditMenu (cmd, menu); }
void MainMenu::buildViewMenu (PopupMenu& menu) { buildViewMenu (cmd, menu); }

void MainMenu::buildWindowMenu (PopupMenu& menu)
{
    if (auto* const cc = dynamic_cast<WorkspacesContentComponent*> (owner.getContentComponent()))
    {
        PopupMenu workspace;
        workspace.addCommandItem (&cmd, Commands::workspaceClassic, "Classic");
        workspace.addCommandItem (&cmd, Commands::workspaceEditing, "Editing");
        workspace.addSeparator();
        workspace.addCommandItem (&cmd, Commands::workspaceResetActive, "Reset to Saved Layout");
        workspace.addCommandItem (&cmd, Commands::workspaceSaveActive, "Save Changes to this Workspace");
        workspace.addItem (80001, "Save as new Workspace", false);
        workspace.addSeparator();
        workspace.addItem (80002, "Edit Workspaces...", false);
        workspace.addSeparator();
        workspace.addCommandItem (&cmd, Commands::workspaceOpen, "Open Workspace File");
        workspace.addCommandItem (&cmd, Commands::workspaceSave, "Save Workspace File");
        menu.addSubMenu ("Workspaces", workspace);
        menu.addSeparator();
        buildWorkspaceMenu (menu);
        menu.addSeparator();
    }

    menu.addCommandItem (&cmd, Commands::hideAllPluginWindows, "Close plugin windows...");
    menu.addCommandItem (&cmd, Commands::showAllPluginWindows, "Show plugin windows...");
}

void MainMenu::buildWorkspaceMenu (PopupMenu& menu)
{
    if (auto* const cc = dynamic_cast<WorkspacesContentComponent*> (owner.getContentComponent()))
        cc->addWorkspaceItemsToMenu (menu);
}

void MainMenu::buildOptionsMenu (PopupMenu& menu)
{
    Settings& settings (world.getSettings());
    settings.addItemsToMenu (world, menu);
}

void MainMenu::buildDebugMenu (PopupMenu& menu)
{
    menu.addItem (1000, "Dump session to console");
    menu.addItem (1111, "Send MIDI Program 1 ch 1");
    menu.addItem (1112, "Send MIDI Program 2 ch 1");
    menu.addItem (1113, "Send MIDI Program 3 ch 1");
    menu.addItem (1114, "Send MIDI Program 4 ch 1");
    menu.addItem (2222, "Show changed status");
    menu.addItem (3333, "Quick Map");
    menu.addItem (4444, "Refresh Mapping Engine");
    menu.addItem (5555, "Toggle Node Channel Strip");
    menu.addItem (6666, "Show Workspace Window");
    menu.addItem (7777, "Save Workspace");
    menu.addItem (7778, "Load Workspace");
    menu.addItem (8000, "Dump License");
    menu.addItem (9000, "MIDI Start");
    menu.addItem (9001, "MIDI Stop");
    menu.addItem (9002, "MIDI Continue");

    menu.addCommandItem (&cmd, Commands::panic, "Panic!");
}

void MainMenu::buildHelpMenu (PopupMenu& menu)
{
    menu.addItem (6000, "User Manual");
    menu.addSeparator();
#ifdef EL_URL_API_LUA_EL
    menu.addItem (6500, "Element Lua API");
#endif
    menu.addSeparator();
    menu.addItem (7000, "Submit Feedback");
#if ! JUCE_MAC
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showAbout, "About Element");
#endif
}

void MainMenu::buildSessionMenu (CommandManager& cmd, PopupMenu& menu)
{
    menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
    menu.addSeparator();

    menu.addCommandItem (&cmd, Commands::sessionOpen, "Open Session...");
    menu.addCommandItem (&cmd, Commands::sessionSave, "Save Session");
    menu.addCommandItem (&cmd, Commands::sessionSaveAs, "Save Session As...");

    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::importGraph, "Import...");
    menu.addCommandItem (&cmd, Commands::exportGraph, "Export graph...");
}

void MainMenu::buildEditMenu (CommandManager& cmd, PopupMenu& menu)
{
#if EL_PRO
    menu.addCommandItem (&cmd, Commands::sessionAddGraph, "New graph");
    menu.addCommandItem (&cmd, Commands::sessionDuplicateGraph, "Duplicate current graph");
    menu.addCommandItem (&cmd, Commands::sessionDeleteGraph, "Delete current graph");
    menu.addSeparator();
#endif
    menu.addCommandItem (&cmd, Commands::undo, "Undo");
    menu.addCommandItem (&cmd, Commands::redo, "Redo");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::cut, "Cut");
    menu.addCommandItem (&cmd, Commands::copy, "Copy");
    menu.addCommandItem (&cmd, Commands::paste, "Paste");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::sessionInsertPlugin, "Insert plugin...");
}

void MainMenu::buildViewMenu (CommandManager& cmd, PopupMenu& menu)
{
    menu.addCommandItem (&cmd, Commands::showPatchBay, "Patch Bay");
    menu.addCommandItem (&cmd, Commands::showGraphEditor, "Graph Editor");
    menu.addSeparator();
#if defined(EL_SOLO)
    menu.addCommandItem (&cmd, Commands::showGraphMixer, "Graph Mixer");
    menu.addCommandItem (&cmd, Commands::showConsole, "Console");
    menu.addSeparator();
#endif
    menu.addCommandItem (&cmd, Commands::rotateContentView, "Rotate View...");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::toggleChannelStrip, "Channel Strip");
    menu.addCommandItem (&cmd, Commands::toggleVirtualKeyboard, "Virtual Keyboard");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showSessionConfig, "Session Properties");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showPluginManager, "Plugin Manager");
    menu.addCommandItem (&cmd, Commands::showKeymapEditor, "Key Mappings");
    menu.addCommandItem (&cmd, Commands::showControllerDevices, "Controllers");
}

void MainMenu::buildPluginMainMenu (CommandManager& cmd, PopupMenu& menu)
{
    buildSessionMenu (cmd, menu);
    menu.addSeparator();
    buildEditMenu (cmd, menu);
    menu.addSeparator();
    buildViewMenu (cmd, menu);

    menu.addCommandItem (&cmd, Commands::showAbout, "About Element");
    menu.addSeparator();
    menu.addItem (99999, "Close all plugin windows...");

#if 0
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
#endif
}
} // namespace Element
