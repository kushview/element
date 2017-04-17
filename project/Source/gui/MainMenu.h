/*
    MainMenu.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_MAIN_MENU_H
#define ELEMENT_MAIN_MENU_H

#include "gui/MainWindow.h"
#include "Commands.h"
#include "CommandManager.h"
#include "URIs.h"

namespace Element {

class MainMenu : public MenuBarModel,
                 public ApplicationCommandTarget
{
public:
    enum RootNames {
        File, Window, Help, NumMenus
    };

    MainMenu (MainWindow& parent, CommandManager& c)
        : owner (parent), cmd(c) { }

    void setupMenu()
    {
      #if JUCE_MAC
        macMenu = new PopupMenu();
        macMenu->addCommandItem (&cmd, Commands::showAbout, "About Element");
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
        const char* const names[] = { "File", "Window", "Help", nullptr };
        return StringArray (names);
    }

    PopupMenu getMenuForIndex (int id, const String&) override
    {
        PopupMenu menu;
        if (id == File)        buildFileMenu (menu);
        else if (id == Window) buildWindowMenu (menu);
        else if (id == Help)   buildHelpMenu (menu);
        else  { };
        return menu;
    }

    void menuItemSelected (int index, int menu) override
    {
#if 0
        if (index == 999)
            owner.app().commander().invokeDirectly(Commands::showPluginManager, true);
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
       #if ! JUCE_MAC
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
#if 0
        ApplicationCommandManager* acm = &owner.app().commander();
        menu.addCommandItem (acm, StandardApplicationCommandIDs::undo, "Undo");
        menu.addCommandItem (acm, StandardApplicationCommandIDs::redo, "Redo");
        menu.addSeparator();
        menu.addCommandItem (acm, StandardApplicationCommandIDs::cut, "Cut");
        menu.addCommandItem (acm, StandardApplicationCommandIDs::copy, "Copy");
        menu.addCommandItem (acm, StandardApplicationCommandIDs::paste, "Paste");
        menu.addSeparator();
        menu.addCommandItem (acm, StandardApplicationCommandIDs::selectAll, "Select All");
#endif
    }
    
    void buildViewMenu (PopupMenu& menu)
    {
        // menu.addCommandItem (&owner.app().commander(), Commands::showLegacyView, "Legacy View");
    }
    
    void buildWindowMenu (PopupMenu& menu)
    {
        // const bool isOpen = owner.app().isWindowOpen (ELEMENT_PLUGIN_MANAGER);
        menu.addCommandItem (&cmd, Commands::showPluginManager);
    }
    
    void buildHelpMenu (PopupMenu& menu)
    {
#if ! JUCE_MAC
       // menu.addCommandItem (&owner.app().commander(), Commands::showAbout, "About Element");
#endif
        // menu.addItem (111, "Do it", true, true);
    }
    
private:
    CommandManager& cmd;
};

}

#endif // ELEMENT_MAIN_MENU_H
