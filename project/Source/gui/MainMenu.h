/*
    MainMenu.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_MAIN_MENU_H
#define ELEMENT_MAIN_MENU_H

#include "URIs.h"
#include "gui/MainWindow.h"

namespace Element {

class MainMenu : public MenuBarModel,
                 public ApplicationCommandTarget
{
public:
    enum RootNames {
        File, Window, Help, NumMenus
    };

    MainMenu (MainWindow& parent)
        : owner (parent) { }

    void setupMenu()
    {
      #if JUCE_MAC
        macMenu = new PopupMenu();
        macMenu->addCommandItem (&owner.app().commander(), Commands::showAbout, "About Element...");
        macMenu->addSeparator();
        macMenu->addCommandItem (&owner.app().commander(), Commands::showPreferences, "Preferences...");
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
        if (index == 999)
            owner.app().commander().invokeDirectly(Commands::showPluginManager, true);
    }
    
    
    // Command Target
    ApplicationCommandTarget* getNextCommandTarget() override { return &owner.app(); }
    void getAllCommands (Array <CommandID>&) override { }
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    bool perform (const InvocationInfo& info) override { return false; }

private:
    MainWindow& owner;
    ScopedPointer<PopupMenu> macMenu;
    
    void buildFileMenu (PopupMenu& menu)
    {
        ApplicationCommandManager* acm = &owner.app().commander();
        menu.addCommandItem (acm, Commands::sessionNew, "New Session");
        menu.addCommandItem (acm, Commands::sessionOpen, "Open Session");
        menu.addCommandItem (acm, Commands::sessionClose, "Close Session");
        menu.addSeparator();
        menu.addCommandItem (acm, Commands::sessionSave, "Save Session...");
        menu.addCommandItem (acm, Commands::sessionSaveAs, "Save Session As...");
        
       #if ! JUCE_MAC
        menu.addCommandItem (&owner.app().commander(), Commands::showPreferences, "Preferences..");
        menu.addSeparator();
        menu.addCommandItem (&owner.app().commander(), StandardApplicationCommandIDs::quit);
       #endif
    }
    
    void buildEditMenu (PopupMenu& menu)
    {
        ApplicationCommandManager* acm = &owner.app().commander();
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
        menu.addCommandItem (&owner.app().commander(), Commands::showLegacyView, "Legacy View");
    }
    
    void buildWindowMenu (PopupMenu& menu)
    {
        const bool isOpen = owner.app().isWindowOpen (ELEMENT_PLUGIN_MANAGER);
        menu.addItem (999, "Plugin Manager", true, isOpen);
    }
    
    void buildHelpMenu (PopupMenu& menu)
    {
        // menu.addItem (111, "Do it", true, true);
    }
};

}

#endif // ELEMENT_MAIN_MENU_H
