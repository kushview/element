/*
    Commands.h - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
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
        workspaceOpen
       #endif
    };
}}
