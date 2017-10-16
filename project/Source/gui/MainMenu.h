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
        File, Window, DebugItem, Help, NumMenus
       #else
        File, Window, Help, NumMenus
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
        const char* const names[] = { "File", "Window", "Debug", "Help", nullptr };
       #else
        const char* const names[] = { "File", "Window", "Help", nullptr };
       #endif
        return StringArray (names, MainMenu::NumMenus);
    }

    PopupMenu getMenuForIndex (int index, const String& name) override
    {
        PopupMenu menu;
        if (name == "File")
            buildFileMenu (menu);
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
    
    PopupMenu makeFileNewMenu()
    {
        PopupMenu menu;
        menu.addItem (2000, "Graph");
        return menu;
    }
    
    void buildFileMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::sessionNew, "New Session");
        menu.addSeparator();

        menu.addCommandItem (&cmd, Commands::mediaOpen, "Open File...");
        menu.addCommandItem (&cmd, Commands::sessionSave, "Save Session");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::signIn, "Manage License");
        
       #if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::checkNewerVersion, "Check For Updates..");
        menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
        menu.addSeparator();
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::quit);
       #endif
    }
    
    void buildEditMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::undo, "Undo");
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::redo, "Redo");
        menu.addSeparator();
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::cut, "Cut");
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::copy, "Copy");
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::paste, "Paste");
        menu.addSeparator();
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::selectAll, "Select All");
    }
    
    void buildViewMenu (PopupMenu& menu)
    {

    }
    
    void buildWindowMenu (PopupMenu& menu)
    {
        menu.addCommandItem (&cmd, Commands::showPluginManager);
    }
    
    void buildDebugMenu (PopupMenu& menu)
    {
        menu.addItem (1000, "Dump Session to Console");
    }
    
    void buildHelpMenu (PopupMenu& menu)
    {
       #if ! JUCE_MAC
        menu.addCommandItem (&cmd, Commands::showAbout, "About Element");
       #endif
    }
    
private:
    Globals& world;
    CommandManager& cmd;
};
}

