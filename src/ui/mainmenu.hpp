// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>

namespace element {

class Commands;
class Content;
class Context;
class MainWindow;

class MainMenu : public MenuBarModel
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

    Content* content();

    // Menu Bar
    StringArray getMenuBarNames() override;
    PopupMenu getMenuForIndex (int index, const String& name) override;
    void menuItemSelected (int index, int menu) override;

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
