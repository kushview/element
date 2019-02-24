/*
    MainMenu.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "JuceHeader.h"

namespace Element {

class CommandManager;
class Globals;
class MainWindow;

class MainMenu : public MenuBarModel,
                 public ApplicationCommandTarget
{
public:
    enum RootNames
    {
         File, 
         Edit, 
         View, 
         Window,
         Options,
        #if JUCE_DEBUG
         DebugItem,
        #endif
         Help, 
         NumMenus
    };

    MainMenu (MainWindow& parent, CommandManager& c);
    ~MainMenu();

    void setupMenu();

    static void buildSessionMenu (CommandManager& cmd, PopupMenu& menu);
    static void buildEditMenu (CommandManager& cmd, PopupMenu& menu);
    static void buildViewMenu (CommandManager& cmd, PopupMenu& menu);
    static void buildPluginMainMenu (CommandManager& cmd, PopupMenu& menu);

    // Menu Bar
    StringArray getMenuBarNames() override;
    PopupMenu getMenuForIndex (int index, const String& name) override;
    void menuItemSelected (int index, int menu) override;
    
    // Command Target
    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; /* &owner.app(); */ }
    void getAllCommands (Array <CommandID>&) override { }
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override { }
    bool perform (const InvocationInfo& info) override { return false; }

private:
    MainWindow& owner;
    Globals& world;
    CommandManager& cmd;
    ScopedPointer<PopupMenu> macMenu;
    const int recentMenuOffset = 20000;
    
    void buildFileMenu (PopupMenu& menu);
    void buildEditMenu (PopupMenu& menu);
    void buildViewMenu (PopupMenu& menu);
    void buildWindowMenu (PopupMenu& menu);
    void buildOptionsMenu (PopupMenu& menu);
    void buildWorkspaceMenu (PopupMenu& menu);
    void buildDebugMenu (PopupMenu& menu);
    void buildHelpMenu (PopupMenu& menu);

    void addRecentFiles (PopupMenu& menu);
};

}
