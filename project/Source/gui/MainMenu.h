/*
    MainMenu.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "gui/MainWindow.h"
#include "gui/ViewHelpers.h"
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
        File, Edit, Window, DebugItem, Help, NumMenus
       #else
        File, Edit, Window, Help, NumMenus
       #endif
    };

    MainMenu (MainWindow& parent, CommandManager& c)
        : owner (parent), world (parent.getWorld()), cmd(c) { }

    void setupMenu()
    {
      #if JUCE_MAC
        macMenu = new PopupMenu();
        macMenu->addCommandItem (&cmd, Commands::showAbout, "About Element");
        macMenu->addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates...");
        macMenu->addSeparator();
        macMenu->addCommandItem (&cmd, Commands::signIn, "Manage License");
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
        const char* const names[] = { "File", "Edit", "Window",
                                      "Debug", "Help", nullptr };
       #else
        const char* const names[] = { "File", "Edit", "Window", "Help", nullptr };
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
        
        if (index == 6000 && menu == Help)
        {
            URL ("https://kushview.net/products/element/").launchInDefaultBrowser();
        }
        else if (index == 7000 && menu == Help)
        {
            URL ("https://kushview.net/feedback/").launchInDefaultBrowser();
        }
        
       #if JUCE_DEBUG
        if (index == 1000)
        {
            DBG("[EL] === SESSION DUMP ===");
            auto data = session->getValueTree().createCopy();
            Node::sanitizeProperties (data, true);
            DBG(data.toXmlString());
        }
       #endif
    }
    
    // Command Target
    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; /* &owner.app(); */ }
    void getAllCommands (Array <CommandID>&) override { }
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    bool perform (const InvocationInfo& info) override { return false; }

private:
    MainWindow& owner;
    ScopedPointer<PopupMenu> macMenu;
    
    void buildFileMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
        menu.addSeparator();

        menu.addCommandItem (&cmd, Commands::sessionOpen, "Open Session...");
        menu.addCommandItem (&cmd, Commands::sessionSave, "Save Session");
        menu.addCommandItem (&cmd, Commands::sessionSaveAs, "Save Session As...");

        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::importGraph, "Import...");
        menu.addCommandItem (&cmd, Commands::exportGraph, "Export graph...");
        
       #if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates..");
        menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
        menu.addCommandItem (&cmd, Commands::signIn, "Manage License");
        menu.addSeparator();
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::quit);
       #endif
    }
    
    void buildEditMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::sessionAddGraph, "New graph");
        menu.addCommandItem (&cmd, Commands::sessionDuplicateGraph, "Duplicate current graph");
        menu.addCommandItem (&cmd, Commands::sessionDeleteGraph, "Delete selected graph");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::copy, "Copy");
        menu.addCommandItem (&cmd, Commands::paste, "Paste");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::sessionInsertPlugin, "Insert plugin...");
    }
    
    void buildWindowMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::showPatchBay, "Patch Bay");
        menu.addCommandItem (&cmd, Commands::showGraphEditor, "Graph Editor");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::rotateContentView, "Rotate View...");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::toggleVirtualKeyboard, "Virtual Keyboard");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showSessionConfig, "Show Session Properties");
        menu.addCommandItem (&cmd, Commands::showGraphConfig, "Show Graph Properties");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showPluginManager, "Plugin Manager");
    }
    
    void buildDebugMenu (PopupMenu& menu)
    {
        menu.addItem (1000, "Dump session to console");
        menu.addItem (1001, "Report bug");
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

