// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/audioengine.hpp>
#include <element/context.hpp>
#include <element/node.hpp>
#include <element/settings.hpp>
#include <element/session.hpp>
#include <element/ui.hpp>
#include <element/ui/content.hpp>
#include <element/ui/commands.hpp>
#include <element/ui/mainwindow.hpp>
#include <element/ui/commands.hpp>

#include "log.hpp"
#include "utils.hpp"
#include "urls.hpp"
#include "services/deviceservice.hpp"
#include "services/mappingservice.hpp"
#include "services/sessionservice.hpp"
#include "ui/mainmenu.hpp"
#include "ui/viewhelpers.hpp"
#include "ui/pluginwindow.hpp"

namespace element {

MainMenu::MainMenu (MainWindow& parent, Commands& c)
    : owner (parent), world (parent.context()), cmd (c) {}

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
    macMenu->addCommandItem (&cmd, Commands::showAbout, Util::appName ("About "));
#if ELEMENT_UPDATER
    macMenu->addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates...");
#endif
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

Content* MainMenu::content()
{
    return dynamic_cast<Content*> (owner.getContentComponent());
}

void MainMenu::menuItemSelected (int index, int menu)
{
    auto session = world.session();
    auto engine = world.audio();

    if (index == 6000 && menu == Help)
    {
        URL (EL_URL_MANUAL_HOME).launchInDefaultBrowser();
    }
    else if (index == 6001 && menu == Help)
    {
#ifdef EL_URL_API_LUA_EL
        URL (EL_URL_API_LUA_EL).launchInDefaultBrowser();
#endif
    }
    else if (index == 6002 && menu == Help)
    {
        URL (EL_URL_DISCUSSIONS).launchInDefaultBrowser();
    }
    else if (index == 7000 && menu == Help)
    {
        URL (EL_URL_BUG_TRACKER).launchInDefaultBrowser();
    }
    else if (index == 7001)
    {
        URL (EL_URL_DONATE).launchInDefaultBrowser();
    }
    else if (index == 7002)
    {
        auto dir = DataPath::defaultSettingsFile().getParentDirectory();
        dir = dir.getChildFile ("log");
        Logger::writeToLog (String ("opening log folder ") + dir.getFullPathName());
        dir.startAsProcess();
    }
    else if (index == 7003)
    {
        URL (EL_URL_CHANGELOG).launchInDefaultBrowser();
    }

    else if (index == 2000 && menu == Window)
    {
        ViewHelpers::closePluginWindows (&owner, false);
    }

    if (menu == Options)
    {
        world.settings().performMenuResult (world, index);
        owner.refreshMenu();
    }

    if (menu == Window)
    {
        if (index >= 100000)
        {
            // if (auto* const cc = dynamic_cast<WorkspacesContentComponent*> (content()))
            //     cc->handleWorkspaceMenuResult (index);
        }
    }

#if JUCE_DEBUG
    if (index == 1000)
    {
        DBG ("[element] === SESSION DUMP ===");
        auto data = session->data().createCopy();
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
        auto& app = owner.services();
        DBG ("has changed: " << (int) app.find<SessionService>()->hasSessionChanged());
    }
    else if (index == 3333)
    {
        auto& app = owner.services();
        if (auto* mapping = app.find<MappingService>())
            mapping->learn (true);
    }
    else if (index == 4444)
    {
        if (session)
            session->cleanOrphanControllerMaps();
        auto& app = owner.services();
        if (auto* devices = app.find<DeviceService>())
            devices->refresh();
        if (auto* gui = app.find<GuiService>())
            gui->stabilizeContent();
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
        juce::ignoreUnused (fileIdx);

        class File f = (*owner.services().find<UI>()).recentFiles().getFile (fileIdx);
        owner.services().find<SessionService>()->openFile (f);
    }
}

void MainMenu::addRecentFiles (PopupMenu& menu)
{
    if (auto* cc = dynamic_cast<Content*> (owner.getContentComponent()))
    {
        PopupMenu recents;
        auto& app (cc->services());
        auto& ui = *app.find<UI>();
        auto& list (ui.recentFiles());
        if (list.getNumFiles() > 0)
        {
            list.createPopupMenuItems (recents, recentMenuOffset, false, true);
            recents.addSeparator();
        }

        recents.addCommandItem (&cmd, Commands::recentsClear, "Clear Recent Files");
        menu.addSubMenu (TRANS ("Open Recent"), recents);
        menu.addSeparator();
    }
}

void MainMenu::buildFileMenu (PopupMenu& menu)
{
    menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::sessionOpen, "Open Session...");
    addRecentFiles (menu);
    menu.addCommandItem (&cmd, Commands::sessionSave, "Save Session");
    menu.addCommandItem (&cmd, Commands::sessionSaveAs, "Save Session As...");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::importGraph, "Import...");
    menu.addCommandItem (&cmd, Commands::exportGraph, "Export graph...");

#if ! JUCE_MAC
    menu.addSeparator();
#if ELEMENT_UPDATER
    menu.addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates..");
#endif
    menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
    menu.addSeparator();
    menu.addCommandItem (&cmd, StandardApplicationCommandIDs::quit);
#endif
}

void MainMenu::buildEditMenu (PopupMenu& menu) { buildEditMenu (cmd, menu); }
void MainMenu::buildViewMenu (PopupMenu& menu)
{
    auto& settings (world.settings());
    buildViewMenu (cmd, menu);
    if (settings.getBool ("legacyControllers", false))
    {
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showControllers, "Controllers");
    }
}

void MainMenu::buildWindowMenu (PopupMenu& menu)
{
    menu.addCommandItem (&cmd, Commands::hideAllPluginWindows, "Close plugin windows...");
    menu.addCommandItem (&cmd, Commands::showAllPluginWindows, "Show plugin windows...");
}

void MainMenu::buildWorkspaceMenu (PopupMenu& menu)
{
}

void MainMenu::buildOptionsMenu (PopupMenu& menu)
{
    Settings& settings (world.settings());
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
    menu.addItem (6666, "Show Workspace Window");
    menu.addItem (7777, "Save Workspace");
    menu.addItem (7778, "Load Workspace");
    menu.addItem (9000, "MIDI Start");
    menu.addItem (9001, "MIDI Stop");
    menu.addItem (9002, "MIDI Continue");

    menu.addCommandItem (&cmd, Commands::panic, "Panic!");
}

void MainMenu::buildHelpMenu (PopupMenu& menu)
{
#if ! JUCE_MAC
    menu.addCommandItem (&cmd, Commands::showAbout, TRANS ("About Element"));
    menu.addSeparator();
#endif
    menu.addItem (6000, TRANS ("User's manual..."));
    menu.addItem (6002, TRANS ("Discussion forum..."));
    menu.addItem (6001, TRANS ("Scripting API..."));
    menu.addSeparator();
    menu.addItem (7002, TRANS ("Log files..."));
    menu.addItem (7000, TRANS ("Issue tracking..."));
    menu.addItem (7003, TRANS ("Change log..."));
#if ! ELEMENT_UPDATER
    menu.addSeparator();
    menu.addItem (7001, TRANS ("Donate..."));
#endif
}

void MainMenu::buildSessionMenu (Commands& cmd, PopupMenu& menu)
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

void MainMenu::buildEditMenu (Commands& cmd, PopupMenu& menu)
{
    menu.addCommandItem (&cmd, Commands::sessionAddGraph, "New graph");
    menu.addCommandItem (&cmd, Commands::sessionDuplicateGraph, "Duplicate current graph");
    menu.addCommandItem (&cmd, Commands::sessionDeleteGraph, "Delete current graph");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::undo, "Undo");
    menu.addCommandItem (&cmd, Commands::redo, "Redo");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::cut, "Cut");
    menu.addCommandItem (&cmd, Commands::copy, "Copy");
    menu.addCommandItem (&cmd, Commands::paste, "Paste");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::sessionInsertPlugin, "Insert plugin...");
}

void MainMenu::buildViewMenu (Commands& cmd, PopupMenu& menu)
{
    menu.addCommandItem (&cmd, Commands::showPatchBay, "Patch Bay");
    menu.addCommandItem (&cmd, Commands::showGraphEditor, "Graph Editor");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showGraphMixer, "Graph Mixer");
    menu.addCommandItem (&cmd, Commands::showConsole, "Console");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::rotateContentView, "Rotate View...");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::toggleChannelStrip, "Channel Strip");
    menu.addCommandItem (&cmd, Commands::toggleVirtualKeyboard, "Virtual Keyboard");
    menu.addCommandItem (&cmd, Commands::toggleMeterBridge, "Meter Bridge");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showSessionConfig, "Session Properties");
    menu.addSeparator();
    menu.addCommandItem (&cmd, Commands::showPluginManager, "Plugin Manager");
    menu.addCommandItem (&cmd, Commands::showKeymapEditor, "Key Mappings");
}

void MainMenu::buildPluginMainMenu (Commands& cmd, PopupMenu& menu)
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
} // namespace element
