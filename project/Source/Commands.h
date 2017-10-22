/*
    Commands.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

namespace Element {
namespace Commands {
    using namespace StandardApplicationCommandIDs;

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

        mediaClose,
        mediaOpen,
        mediaNew,
        mediaSave,
        mediaSaveAs,

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

        checkNewerVersion      = 0x0500,

        signIn,
        signOut,
        
        transportRewind        = 0x0600,
        transportForward,
        transportPlay,
        transportRecord,
        transportSeekZero,
        transportStop
   };

   namespace Categories
   {
       static const char* const application   = "Application";
       static const char* const general       = "General";
       static const char* const editing       = "Editing";
       static const char* const view          = "View";
       static const char* const windows       = "Windows";
       static const char* const session       = "Session";
   }
    
    inline void getApplicationCommands (Array<CommandID>& commands)
    {
        const CommandID cmds[] = {
            Commands::exportAudio,
            Commands::exportMidi,
            
            Commands::sessionClose,
            Commands::sessionNew,
            Commands::sessionOpen,
            Commands::sessionSave,
            Commands::sessionSaveAs,
            Commands::sessionAddGraph,
            
            Commands::showAbout,
            Commands::showLegacyView,
            Commands::showPreferences,
            Commands::showPluginManager,
            
            Commands::mediaClose,
            Commands::mediaSave,
            Commands::mediaSaveAs,
            
            Commands::checkNewerVersion,
            
            Commands::signIn,
            Commands::signOut,
            
            Commands::transportRewind,
            Commands::transportForward,
            Commands::transportPlay,
            Commands::transportRecord,
            Commands::transportSeekZero,
            Commands::transportStop,
            
            Commands::quit,
            Commands::undo,
            Commands::redo,
            Commands::cut,
            Commands::copy,
            Commands::paste,
            Commands::selectAll
        };
        
        commands.addArray (cmds, numElementsInArray (cmds));
    }
    
    inline void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
    {
        typedef ApplicationCommandInfo Info;
        switch (commandID)
        {
            case Commands::exportAudio:
                result.setInfo ("Export Audio", "Export to an audio file", "Exporting", 0);
                break;
            case Commands::exportMidi:
                result.setInfo ("Exort MIDI", "Export to a MIDI file", "Exporting", 0);
                break;
            case Commands::importGraph:
                result.setInfo ("Import Graph", "Import graph to current session", "Exporting", 0);
                break;
            case Commands::exportGraph:
                result.setInfo ("Exort Graph", "Export graph to file", "Exporting", 0);
                break;
                
            // MARK: Session Commands
            case Commands::sessionClose:
                result.setInfo ("Close Session", "Close the current session", "Session", 0);
                break;
            case Commands::sessionNew:
                result.addDefaultKeypress ('n', ModifierKeys::commandModifier);
                result.setInfo ("New Session", "Create a new session", "Session", 0);
                break;
            case Commands::sessionOpen:
                result.addDefaultKeypress ('o', ModifierKeys::commandModifier);
                result.setInfo ("Open Session", "Open an existing session", "Session", 0);
                break;
            case Commands::sessionSave:
                result.addDefaultKeypress ('s', ModifierKeys::commandModifier);
                result.setInfo ("Save Session", "Save the current session", Categories::session, 0);
                break;
            case Commands::sessionSaveAs:
                result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
                result.setInfo ("Save Session As", "Save the current session with a new name", Categories::session, 0);
                break;
            case Commands::sessionAddGraph:
                result.addDefaultKeypress('n', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
                result.setInfo ("Add Graph", "Add a new graph to the session", Categories::session, 0);
                break;
            case Commands::sessionDuplicateGraph:
                result.addDefaultKeypress ('d', ModifierKeys::shiftModifier | ModifierKeys::commandModifier);
                result.setInfo ("Duplicate Graph", "Duplicate the current graph", Categories::session, 0);
                break;
            case Commands::sessionDeleteGraph:
                result.addDefaultKeypress (KeyPress::backspaceKey, ModifierKeys::commandModifier);
                result.setInfo ("Delete Graph", "Deletes the current graph", Categories::session, 0);
                break;
            case Commands::sessionInsertPlugin:
                result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
                result.setInfo ("Insert Plugin", "Add a plugin in the current graph", Categories::session, Info::isDisabled);
                break;

            // MARK: Media Commands
            case Commands::mediaNew:
                result.setInfo ("New Media", "Close the current media", "Application", 0);
                break;
            case Commands::mediaClose:
                result.setInfo ("Close Media", "Close the current media", "Application", 0);
                break;
            case Commands::mediaOpen:
                result.setInfo ("Open Media", "Opens a type of supported media", "Session Media", 0);
                break;
            case Commands::mediaSave:
                result.setInfo ("Close Media", "Saves the currently viewed object", "Session Media", 0);
                break;
            case Commands::mediaSaveAs:
                result.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
                result.setInfo ("Save Media As", "Saves the current object with another name", "Session Media", 0);
                break;

            // MARK: Show Commands
            case Commands::showPreferences:
                result.setInfo ("Show Preferences", "Element Preferences", "Application", 0);
                result.addDefaultKeypress (',', ModifierKeys::commandModifier);
                break;
            case Commands::showAbout:
                result.setInfo ("Show About", "About this program", "Application", 0);
                break;
            case Commands::showLegacyView:
                result.setInfo ("Legacy View", "Shows the legacy Beat Thang Virtual GUI", "Interfaces", 0);
                break;
            case Commands::showPluginManager:
                result.setInfo ("Plugin Manager", "Element Plugin Management", "Application", 0);
                break;
            case Commands::showSessionConfig:
                result.setInfo ("Session Settings", "Session Settings", "Session", 0);
                break;
            case Commands::showGraphConfig:
                result.setInfo ("Graph Settings", "Graph Settings", "Session", Info::isDisabled);
                break;
            case Commands::showPatchBay:
                result.addDefaultKeypress (KeyPress::F1Key, 0);
                result.setInfo ("Patch Bay", "Show the patch bay", "Session", 0);
                break;
            case Commands::showGraphEditor:
                result.addDefaultKeypress (KeyPress::F2Key, 0);
                result.setInfo ("Graph Editor", "Show the graph editor", "Session", 0);
                break;
            
            case Commands::checkNewerVersion:
                result.setInfo ("Check For Updates", "Check newer version", "Application", 0);
                break;
                
            case Commands::signIn:
                result.setInfo ("Sign In", "Saves the current object with another name", "Application", 0);
                break;
            case Commands::signOut:
                result.setInfo ("Sign Out", "Saves the current object with another name", "Application",   0);
                break;
                
            case Commands::quit:
                result.setInfo ("Quit", "Quit the app", "Application", 0);
                result.addDefaultKeypress ('q', ModifierKeys::commandModifier);
                break;
            case Commands::undo:
                result.setInfo ("Undo", "Undo the last operation", "Application", 0);
                break;
            case Commands::redo:
                result.setInfo ("Redo", "Redo the last operation", "Application", 0);
                break;
            case Commands::cut:
                result.setInfo ("Cut", "Cut", "Application", 0);
                break;
            case Commands::copy:
                result.addDefaultKeypress ('c', ModifierKeys::commandModifier);
                result.setInfo ("Copy", "Copy", "Application", Info::isDisabled);
                break;
            case Commands::paste:
                result.addDefaultKeypress ('p', ModifierKeys::commandModifier);
                result.setInfo ("Paste", "Paste", "Application", Info::isDisabled);
                break;
            case Commands::selectAll:
                result.setInfo ("Select All", "Select all", "Application", 0);
                break;
                
            case Commands::transportRewind:
                result.setInfo ("Rewind", "Transport Rewind", "Playback", 0);
                result.addDefaultKeypress ('j', 0);
                break;
            case Commands::transportForward:
                result.setInfo ("Forward", "Transport Fast Forward", "Playback", 0);
                result.addDefaultKeypress ('l', 0);
                break;
            case Commands::transportPlay:
                result.setInfo ("Play", "Transport Play", "Playback", 0);
                result.addDefaultKeypress (KeyPress::spaceKey, 0);
                break;
            case Commands::transportRecord:
                result.setInfo ("Record", "Transport Record", "Playback", 0);
                break;
            case Commands::transportSeekZero:
                result.setInfo ("Seek Start", "Seek to Beginning", "Playback", 0);
                break;
            case Commands::transportStop:
                result.setInfo ("Stop", "Transport Stop", "Playback", 0);
                break;
        }
    }
}}
