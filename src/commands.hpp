/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include "ElementApp.h"

namespace element {

class Commands : public juce::ApplicationCommandManager
{
public:
    Commands() {}
    ~Commands() {}

    enum IDs : juce::CommandID
    {
        invalid = -1,

        showAbout = 0x0100,
        showLegacyView,
        showPluginManager,
        showPreferences,
        showSessionConfig,
        showGraphConfig,
        showPatchBay,
        showGraphEditor,
        showLastContentView,
        showAllPluginWindows,
        showKeymapEditor,
        hideAllPluginWindows,

        toggleVirtualKeyboard,
        rotateContentView,

        mediaClose,
        mediaOpen,
        mediaNew,
        mediaSave,
        mediaSaveAs,

        showControllers,
        toggleUserInterface,
        toggleChannelStrip,
        showGraphMixer,
        showConsole,
        toggleMeterBridge,

        sessionClose = 0x0300,
        sessionOpen,
        sessionNew,
        sessionSave,
        sessionSaveAs,
        sessionAddGraph,
        sessionDuplicateGraph = 900,
        sessionDeleteGraph = 901,
        sessionInsertPlugin = 902,

        exportAudio = 0x0400,
        exportMidi,
        exportGraph,
        importGraph,

        panic,
        importSession,

        checkNewerVersion = 0x0500,

        signIn,
        signOut,

        transportRewind = 0x0600,
        transportForward,
        transportPlay,
        transportRecord,
        transportSeekZero,
        transportStop,

        graphNew = 0x0700,
        graphOpen,
        graphSave,
        graphSaveAs,

        workspaceSave = 0x0800,
        workspaceOpen,
        workspaceResetActive,
        workspaceSaveActive,

        workspaceClassic = 0x0900,
        workspaceEditing,

        recentsClear = 0x1000,

        quit = juce::StandardApplicationCommandIDs::quit,
        copy = juce::StandardApplicationCommandIDs::copy,
        undo = juce::StandardApplicationCommandIDs::undo,
        redo = juce::StandardApplicationCommandIDs::redo,
        paste = juce::StandardApplicationCommandIDs::paste,
        cut = juce::StandardApplicationCommandIDs::cut,
        selectAll = juce::StandardApplicationCommandIDs::selectAll
    };

    inline static Array<juce::CommandID> getAllCommands()
    {
        return {
            copy,
            quit,
            undo,
            redo,
            paste,
            cut,
            selectAll,
            showAbout,
            showLegacyView,
            showPluginManager,
            showPreferences,
            showSessionConfig,
            showGraphConfig,
            showPatchBay,
            showGraphEditor,
            showLastContentView,
            showAllPluginWindows,
            showKeymapEditor,
            hideAllPluginWindows,

            toggleVirtualKeyboard,
            rotateContentView,

            mediaClose,
            mediaOpen,
            mediaNew,
            mediaSave,
            mediaSaveAs,

            showControllers,
            toggleUserInterface,
            toggleChannelStrip,
            showGraphMixer,
            showConsole,

            sessionClose,
            sessionOpen,
            sessionNew,
            sessionSave,
            sessionSaveAs,
            sessionAddGraph,
            sessionDuplicateGraph,
            sessionDeleteGraph,
            sessionInsertPlugin,

            exportAudio,
            exportMidi,
            exportGraph,
            importGraph,

            panic,
            importSession,

            checkNewerVersion,

            signIn,
            signOut,

            transportRewind,
            transportForward,
            transportPlay,
            transportRecord,
            transportSeekZero,
            transportStop,

            graphNew,
            graphOpen,
            graphSave,
            graphSaveAs,

            workspaceSave,
            workspaceOpen,
            workspaceResetActive,
            workspaceSaveActive,
            workspaceClassic,
            workspaceEditing,

            recentsClear,
        };
    }

    inline static String toString (juce::CommandID command)
    {
        switch (command)
        {
            case Commands::quit:
                return "quit";
                break;
            case Commands::undo:
                return "undo";
                break;
            case Commands::redo:
                return "redo";
                break;
            case Commands::showAbout:
                return "showAbout";
                break;
            case Commands::showLegacyView:
                return "showLegacyView";
                break;
            case Commands::showPluginManager:
                return "showPluginManager";
                break;
            case Commands::showPreferences:
                return "showPreferences";
                break;
            case Commands::showSessionConfig:
                return "showSessionConfig";
                break;
            case Commands::showGraphConfig:
                return "showGraphConfig";
                break;
            case Commands::showPatchBay:
                return "showPatchBay";
                break;
            case Commands::showGraphEditor:
                return "showGraphEditor";
                break;
            case Commands::showLastContentView:
                return "showLastContentView";
                break;
            case Commands::showAllPluginWindows:
                return "showAllPluginWindows";
                break;
            case Commands::showKeymapEditor:
                return "showKeymapEditor";
                break;
            case Commands::hideAllPluginWindows:
                return "hideAllPluginWindows";
                break;
            case Commands::toggleVirtualKeyboard:
                return "toggleVirtualKeyboard";
                break;
            case Commands::rotateContentView:
                return "rotateContentView";
                break;
            case Commands::showControllers:
                return "showControllers";
                break;
            case Commands::toggleUserInterface:
                return "toggleUserInterface";
                break;
            case Commands::toggleChannelStrip:
                return "toggleChannelStrip";
                break;
            case Commands::showGraphMixer:
                return "showGraphMixer";
                break;
            case Commands::showConsole:
                return "showConsole";
                break;
            case Commands::panic:
                return "panic";
                break;
            case Commands::graphNew:
                return "graphNew";
                break;
            case Commands::graphOpen:
                return "graphOpen";
                break;
            case Commands::graphSave:
                return "graphSave";
                break;
            case Commands::graphSaveAs:
                return "graphSaveAs";
                break;
            case Commands::recentsClear:
                return "recentsClear";
                break;
            default:
                break;
        }

        return {};
    }

    inline static juce::CommandID fromString (const String& str)
    {
        if (str == "quit")
            return Commands::quit;

        if (str == "showAbout")
            return Commands::showAbout;
        if (str == "showLegacyView")
            return Commands::showLegacyView;
        if (str == "showPluginManager")
            return Commands::showPluginManager;
        if (str == "showPreferences")
            return Commands::showPreferences;
        if (str == "showSessionConfig")
            return Commands::showSessionConfig;
        if (str == "showGraphConfig")
            return Commands::showGraphConfig;
        if (str == "showPatchBay")
            return Commands::showPatchBay;
        if (str == "showGraphEditor")
            return Commands::showGraphEditor;
        if (str == "showLastContentView")
            return Commands::showLastContentView;
        if (str == "showAllPluginWindows")
            return Commands::showAllPluginWindows;
        if (str == "showKeymapEditor")
            return Commands::showKeymapEditor;
        if (str == "hideAllPluginWindows")
            return Commands::hideAllPluginWindows;

        if (str == "toggleVirtualKeyboard")
            return Commands::toggleVirtualKeyboard;
        if (str == "rotateContentView")
            return Commands::rotateContentView;

        if (str == "showControllers")
            return Commands::showControllers;
        if (str == "toggleUserInterface")
            return Commands::toggleUserInterface;
        if (str == "toggleChannelStrip")
            return Commands::toggleChannelStrip;
        if (str == "showGraphMixer")
            return Commands::showGraphMixer;
        if (str == "showConsole")
            return Commands::showConsole;

        if (str == "panic")
            return Commands::panic;

        if (str == "graphNew")
            return Commands::graphNew;
        if (str == "graphOpen")
            return Commands::graphOpen;
        if (str == "graphSave")
            return Commands::graphSave;
        if (str == "graphSaveAs")
            return Commands::graphSaveAs;

        if (str == "recentsClear")
            return Commands::recentsClear;
        return Commands::invalid;
    }

    inline static String toOSCAddress (juce::CommandID command)
    {
        auto commandStr = toString (command);
        if (commandStr.isEmpty())
            return {};

        String addy = "/element/command/";
        addy << commandStr;
        return addy;
    }

    inline static StringArray getOSCAddresses()
    {
        StringArray res;
        for (auto command : getAllCommands())
        {
            const auto addy = toOSCAddress (command);
            if (addy.isNotEmpty())
                res.add (addy);
        }
        return res;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Commands)
};

} // namespace element
