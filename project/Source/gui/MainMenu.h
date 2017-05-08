/*
    MainMenu.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_MAIN_MENU_H
#define ELEMENT_MAIN_MENU_H

#include "gui/MainWindow.h"
#include "gui/ViewHelpers.h"
#include "session/CommandManager.h"
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
        : owner (parent), cmd(c) { }

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

    PopupMenu getMenuForIndex (int, const String& name) override
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
        #if 0
        if (index == 999)
            owner.app().commander().invokeDirectly(Commands::showPluginManager, true);
        #endif
        
        if (index == 1001) {
            ViewHelpers::postMessageFor (&owner, new Message());
        }
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
//        menu.addSubMenu ("New", makeFileNewMenu());
//        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::mediaOpen, "Open");
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::mediaSave, "Save");
        
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::signIn, "Authorize...");
        
       #if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem (&cmd, Commands::showPreferences, "Check For Updates..");
        menu.addCommandItem (&cmd, Commands::showPreferences, "Preferences..");
        menu.addSeparator();
        menu.addCommandItem (&cmd, StandardApplicationCommandIDs::quit);
       #endif
       
       #if 0
        ApplicationCommandManager* acm = &owner.app().commander();
        menu.addCommandItem (acm, Commands::sessionNew, "New Session");
        menu.addCommandItem (acm, Commands::sessionOpen, "Open Session");
        menu.addCommandItem (acm, Commands::sessionClose, "Close Session");
        menu.addSeparator();
        menu.addCommandItem (acm, Commands::sessionSave, "Save Session...");
        menu.addCommandItem (acm, Commands::sessionSaveAs, "Save Session As...");
       #endif
    }
    
    void buildEditMenu (PopupMenu& menu)
    {
        ApplicationCommandManager* acm = &cmd;
        menu.addCommandItem (acm, StandardApplicationCommandIDs::undo, "Undo");
        menu.addCommandItem (acm, StandardApplicationCommandIDs::redo, "Redo");
        menu.addSeparator();
        menu.addCommandItem (acm, StandardApplicationCommandIDs::cut, "Cut");
        menu.addCommandItem (acm, StandardApplicationCommandIDs::copy, "Copy");
        menu.addCommandItem (acm, StandardApplicationCommandIDs::paste, "Paste");
        menu.addSeparator();
        menu.addCommandItem (acm, StandardApplicationCommandIDs::selectAll, "Select All");
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
        menu.addItem (1000, "Reset settings file");
        menu.addItem (1001, "Authenticate Trial");
    }
    
    void buildHelpMenu (PopupMenu& menu)
    {
       #if ! JUCE_MAC
        menu.addCommandItem (&cmd, Commands::showAbout, "About Element");
       #endif
    }
    
private:
    CommandManager& cmd;
};

}

#endif // ELEMENT_MAIN_MENU_H
