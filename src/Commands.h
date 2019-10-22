/*
    Commands.h - This file is part of Element
    Copyright (c) 2019 Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "ElementApp.h"

namespace Element {
namespace Commands {
    using namespace StandardApplicationCommandIDs;

    namespace Categories
    {
        static const char* const Application   = "Application";
        static const char* const General       = "General";
        static const char* const Editing       = "Editing";
        static const char* const View          = "View";
        static const char* const Windows       = "Windows";
        static const char* const Session       = "Session";
        static const char* const UserInterface = "User Interface";
    }

    enum AppCommands
    {
        showAbout              = 0x0100,
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

        showControllerDevices,
        toggleUserInterface,
        toggleChannelStrip,
        showGraphMixer,
        
        sessionClose           = 0x0300,
        sessionOpen,
        sessionNew,
        sessionSave,
        sessionSaveAs,
        sessionAddGraph,
        sessionDuplicateGraph = 900,
        sessionDeleteGraph    = 901,
        sessionInsertPlugin   = 902,

        exportAudio            = 0x0400,
        exportMidi,
        exportGraph,
        importGraph,

        panic,
        importSession,

        checkNewerVersion      = 0x0500,

        signIn,
        signOut,

        transportRewind        = 0x0600,
        transportForward,
        transportPlay,
        transportRecord,
        transportSeekZero,
        transportStop,

        graphNew               = 0x0700,
        graphOpen,
        graphSave,
        graphSaveAs,

       #if EL_DOCKING
        workspaceSave          = 0x0800,
        workspaceOpen,
        workspaceResetActive,
        workspaceSaveActive,

        workspaceClassic       = 0x0900,
        workspaceEditing,
       #endif
    };
}}
