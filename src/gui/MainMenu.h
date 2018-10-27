/*
    MainMenu.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once
#include "controllers/DevicesController.h"
#include "controllers/MappingController.h"
#include "controllers/SessionController.h"
#include "controllers/GuiController.h"
#include "gui/MainWindow.h"
#include "gui/ViewHelpers.h"
#include "gui/PluginWindow.h"

#include "engine/AudioEngine.h"

#include "session/Session.h"
#include "session/CommandManager.h"
#include "session/Node.h"
#include "session/UnlockStatus.h"
#include "Commands.h"
#include "Globals.h"

namespace Element {

class MainMenu : public MenuBarModel,
                 public ApplicationCommandTarget
{
public:
    enum RootNames
    {
       #if JUCE_DEBUG
        File, Edit, View, Window, DebugItem, Help, NumMenus
       #else
        File, Edit, View, Window, Help, NumMenus
       #endif
    };

    MainMenu (MainWindow& parent, CommandManager& c)
        : owner (parent), world (parent.getWorld()), cmd (c) { }

    void setupMenu()
    {
      #if JUCE_MAC
        macMenu = new PopupMenu();
        macMenu->addCommandItem (&cmd, Commands::showAbout, "About Element");
        macMenu->addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates...");
        macMenu->addSeparator();
        macMenu->addCommandItem (&cmd, Commands::showPreferences, "Preferences...");
        MenuBarModel::setMacMainMenu (this, macMenu.get());
       #else
        owner.setMenuBar (this);
       #endif
    }

    ~MainMenu()
    {
       #if JUCE_MAC
        MainMenu::setMacMainMenu (nullptr);
        macMenu = nullptr;
       #else
        owner.setMenuBar (nullptr);
       #endif
    }

    StringArray getMenuBarNames() override
    {
       #if JUCE_DEBUG
        const char* const names[] = { "File", "Edit", "View", "Window", "Debug", "Help", nullptr };
       #else
        const char* const names[] = { "File", "Edit", "View", "Window", "Help", nullptr };
       #endif
        return StringArray (names, MainMenu::NumMenus);
    }

    PopupMenu getMenuForIndex (int index, const String& name) override
    {
        PopupMenu menu;
        
        if (name == "File")
            buildFileMenu (menu);
        else if (name == "Edit")
            buildEditMenu (menu);
        else if (name == "View")
            buildViewMenu (menu);
        else if (name == "Window")
            buildWindowMenu (menu);
        else if (name == "Help")
            buildHelpMenu (menu);
       #if JUCE_DEBUG
        else if (name == "Debug")
            buildDebugMenu (menu);
       #endif

        return menu;
    }

    void menuItemSelected (int index, int menu) override
    {
        auto session = world.getSession();
        auto engine  = world.getAudioEngine();

        if (index == 6000 && menu == Help)
        {
            URL ("http://help.kushview.net/collection/10-element").launchInDefaultBrowser();
        }
        else if (index == 7000 && menu == Help)
        {
            URL ("https://kushview.net/feedback/").launchInDefaultBrowser();
        }
        else if (index == 2000 && menu == Window)
        {
            ViewHelpers::closePluginWindows (&owner, false);
        }
        
       #if JUCE_DEBUG
        if (index == 1000)
        {
            DBG("[EL] === SESSION DUMP ===");
            auto data = session->getValueTree().createCopy();
            Node::sanitizeProperties (data, true);
            DBG(data.toXmlString());
        }
        else if (index == 1111)
        {
            auto msg = MidiMessage::programChange (1, 10);
            msg.setTimeStamp (1.0 + Time::getMillisecondCounterHiRes());
            engine->addMidiMessage (msg);
        }
        else if (index == 2222)
        {
            auto&  app = owner.getAppController();
            DBG("has changed: " << (int) app.findChild<SessionController>()->hasSessionChanged());
        }
        else if (index == 3333)
        {
            auto&  app = owner.getAppController();
            if (auto* mapping = app.findChild<MappingController>())
                mapping->learn (true);
        }
        else if (index == 4444)
        {
            if (auto session = owner.getAppController().getWorld().getSession())
                session->cleanOrphanControllerMaps();
            auto& app = owner.getAppController();
            if (auto* devices = app.findChild<DevicesController>())
                devices->refresh();
            if (auto* gui = app.findChild<GuiController>())
                gui->stabilizeContent();
        }
        else if (index == 5555)
        {
            auto *cc = ViewHelpers::findContentComponent (&owner);
            cc->setNodeChannelStripVisible (! cc->isNodeChannelStripVisible());
        }
        #endif
        
        if (menu == File && index >= recentMenuOffset)
        {
            const int fileIdx = index - recentMenuOffset;
            class File f = owner.getAppController().getRecentlyOpenedFilesList().getFile (fileIdx);
            owner.getAppController().findChild<SessionController>()->openFile (f);
        }
    }
    
    // Command Target
    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; /* &owner.app(); */ }
    void getAllCommands (Array <CommandID>&) override { }
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    bool perform (const InvocationInfo& info) override { return false; }
    
    static void buildSessionMenu (CommandManager& cmd, PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
        menu.addSeparator();
        
        menu.addCommandItem (&cmd, Commands::sessionOpen,   "Open Session...");
        menu.addCommandItem (&cmd, Commands::sessionSave,   "Save Session");
        menu.addCommandItem (&cmd, Commands::sessionSaveAs, "Save Session As...");
        
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::importGraph, "Import...");
        menu.addCommandItem (&cmd, Commands::exportGraph, "Export graph...");
    }
    
    static void buildEditMenu (CommandManager& cmd, PopupMenu& menu)
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
    
    static void buildViewMenu (CommandManager& cmd, PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::showPatchBay, "Patch Bay");
        menu.addCommandItem (&cmd, Commands::showGraphEditor, "Graph Editor");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::rotateContentView, "Rotate View...");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::toggleChannelStrip, "Channel Strip");
        menu.addCommandItem (&cmd, Commands::toggleVirtualKeyboard, "Virtual Keyboard");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showSessionConfig, "Session Properties");
        menu.addCommandItem (&cmd, Commands::showGraphConfig, "Graph Properties");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showPluginManager, "Plugin Manager");
        menu.addCommandItem (&cmd, Commands::showKeymapEditor, "Key Mappings");
        menu.addCommandItem (&cmd, Commands::showControllerDevices, "Controllers");
    }
    
    static void buildPluginMainMenu (CommandManager& cmd, PopupMenu& menu)
    {
        buildSessionMenu (cmd, menu);
        menu.addSeparator();
        buildEditMenu(cmd, menu);
        menu.addSeparator();
        buildViewMenu(cmd, menu);
        menu.addCommandItem (&cmd, Commands::showAbout, "About Element");
        menu.addSeparator();
        menu.addItem (99999, "Close all plugin windows...");
       #if 0
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
       #endif
    }
    
private:
    MainWindow& owner;
    ScopedPointer<PopupMenu> macMenu;
    const int recentMenuOffset = 20000;
    
    void buildFileMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
        menu.addSeparator();

        menu.addCommandItem (&cmd, Commands::sessionOpen, "Open Session...");
  
        if (auto* cc = dynamic_cast<ContentComponent*> (owner.getContentComponent()))
        {
            PopupMenu recents;
            auto& app (cc->getAppController());
            auto& list (app.getRecentlyOpenedFilesList());
            list.createPopupMenuItems (recents, recentMenuOffset, true, true);
            menu.addSubMenu ("Open Recent", recents);
            menu.addSeparator();
        }
        
        menu.addCommandItem (&cmd, Commands::sessionSave, "Save Session");
        menu.addCommandItem (&cmd, Commands::sessionSaveAs, "Save Session As...");

        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::importGraph, "Import...");
        menu.addCommandItem (&cmd, Commands::exportGraph, "Export graph...");
        
       #if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates..");
        menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
        menu.addSeparator();
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::quit);
       #endif
    }
    
    void buildEditMenu (PopupMenu& menu) { buildEditMenu (cmd, menu); }
    void buildViewMenu (PopupMenu& menu) { buildViewMenu (cmd, menu); }
    void buildWindowMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::hideAllPluginWindows, "Close plugin windows...");
        menu.addCommandItem (&cmd, Commands::showAllPluginWindows, "Show plugin windows...");
    }
    
    void buildDebugMenu (PopupMenu& menu)
    {
        menu.addItem (1000, "Dump session to console");
        menu.addItem (1111, "Report bug");
        menu.addItem (2222, "Show changed status");
        menu.addItem (3333, "Quick Map");
        menu.addItem (4444, "Refresh Mapping Engine");
        menu.addItem (5555, "Toggle Node Channel Strip");
        menu.addCommandItem (&cmd, Commands::panic, "Panic!");
    }
    
    void buildHelpMenu (PopupMenu& menu)
    {
        menu.addItem (6000, "Online documentation...");
        menu.addItem (7000, "Submit Feedback...");
       #if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showAbout, "About Element");
       #endif
    }
    
private:
    Globals& world;
    CommandManager& cmd;
};
}

