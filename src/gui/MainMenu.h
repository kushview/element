/*
    This file is part of Element
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.

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

#pragma once

#include <element/juce.hpp>

namespace element {

class Commands;
class ContentComponent;
class Context;
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
        Options,
        Window,
#if JUCE_DEBUG
        DebugItem,
#endif
        Help,
        NumMenus
    };

    MainMenu (MainWindow& parent, Commands& c);
    ~MainMenu();

    void setupMenu();

    static void buildSessionMenu (Commands& cmd, PopupMenu& menu);
    static void buildEditMenu (Commands& cmd, PopupMenu& menu);
    static void buildViewMenu (Commands& cmd, PopupMenu& menu);
    static void buildPluginMainMenu (Commands& cmd, PopupMenu& menu);

    ContentComponent* getContentComponent();

    // Menu Bar
    StringArray getMenuBarNames() override;
    PopupMenu getMenuForIndex (int index, const String& name) override;
    void menuItemSelected (int index, int menu) override;

    // Command Target
    ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands (Array<CommandID>&) override {}
    void getCommandInfo (CommandID, ApplicationCommandInfo&) override {}
    bool perform (const InvocationInfo& info) override { return false; }

private:
    MainWindow& owner;
    Context& world;
    Commands& cmd;
    std::unique_ptr<PopupMenu> macMenu;
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

} // namespace element
