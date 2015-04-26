/*
    Commands.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_GUI_COMMANDS_H
#define ELEMENT_GUI_COMMANDS_H

#include <element/Juce.h>

namespace Element {
namespace Commands {

    /** Command IDs that map to a BeatThang (gen 1) Hardware Device
        some of these are also the same command id's as regular app commands */
    enum DeviceButtons
    {
        deviceSong          = 0x8000,
        devicePattern       = 0x8001,
        deviceKit           = 0x8002,
        deviceInst          = 0x8003,

        deviceFreak         = 0x8010,
        deviceDelay         = 0x8011,
        deviceVerb          = 0x8012,
        deviceBang          = 0x8013,
        deviceBlang         = 0x8014,

        deviceEdit          = 0x8020,
        deviceSave          = 0x8021,
        deviceExit          = 0x8022,
        deviceUndo          = 0x8023,
        deviceInsCopy       = 0x8024,
        deviceDelete        = 0x8025,
        deviceExport        = 0x8026,
        deviceSystem        = 0x8027,
        deviceVolumes       = 0x8028,
        deviceSample        = 0x8029,

        deviceSeekStart     = 0x8030,
        deviceSeekBack      = 0x8031,
        deviceSeekForward   = 0x8032,

        deviceRecord        = 0x8040,
        deviceStop          = 0x8041,
        devicePlay          = 0x8042,

        deviceTrack         = 0x8050,

        deviceNineToSixteen = 0x8060,
        deviceThru          = 0x8061,
        deviceMute          = 0x8062,
        deviceSolo          = 0x8063,

        deviceMixer         = 0x8070,
        deviceBankUp        = 0x8071,
        deviceBankDown      = 0x8072,

        deviceTempo         = 0x8073,
        deviceRoll          = 0x8074,
        deviceHold          = 0x8075,

        // Pads
        devicePadPress      = 0x8100,
        devicePadRelease    = 0x8120

    };

    enum AppCommands
    {
        showAbout              = 0x9000,
        showLegacyView         = 0x9001,
        showPluginManager      = 0x9002,
        showPreferences        = 0x9003,

        mediaClose             = 0x9010,
        mediaOpen              = 0x9011,
        mediaNew               = 0x9012,
        mediaSave              = 0x9013,
        mediaSaveAs            = 0x9014,

        sessionClose           = 0x9020,
        sessionOpen            = 0x9021,
        sessionNew             = 0x9022,
        sessionSave            = 0x9023,
        sessionSaveAs          = 0x9024,

        exportAudio            = 0x9030,
        exportMidi             = 0x9031,

        transportRewind        = deviceSeekBack,
        transportForward       = deviceSeekForward,
        transportPlay          = devicePlay,
        transportRecord        = deviceRecord,
        transportSeekZero      = deviceSeekStart,
        transportStop          = deviceStop
   };

   namespace Categories
   {
       static const char* const general       = "General";
       static const char* const editing       = "Editing";
       static const char* const view          = "View";
       static const char* const windows       = "Windows";
   }

   inline bool isDeviceTrackCommand (CommandID cmd) {
       return (cmd >= Commands::deviceTrack && cmd < Commands::deviceTrack + 16);
   }

   inline void getDevicePadCommands (Array<CommandID>& commands)
   {
       // Add the pad press commands.
       CommandID padCommandIds [13 * 2];
       for (int i = 0, j = 0; i < 13; ++i)
       {
           padCommandIds [j++] = Commands::devicePadPress + i;
           padCommandIds [j++] = Commands::devicePadRelease + i;
       }

       commands.addArray(const_cast<const CommandID*> (padCommandIds),
                    numElementsInArray (padCommandIds));
   }

   inline void getDeviceTrackCommands (Array<CommandID>& commands)
   {
       for (int i = Commands::deviceTrack; i < Commands::deviceTrack + 16;) {
           commands.add (i);
        ++i;
       }
   }

   inline bool getDeviceTrackInfo (CommandID maybeTrackCommand, ApplicationCommandInfo& result)
   {
       if (! isDeviceTrackCommand (maybeTrackCommand))
           return false;

       // this is one based ....

       const var track = String (maybeTrackCommand - Commands::deviceTrack + 1);

       const int pressOffset = KeyPress::numberPad0;
       const int trackOffset = int(track) > 8 ? int(track) - 8 : int(track);
       ModifierKeys mkeys = int(track) <= 8 ? ModifierKeys::noModifiers : ModifierKeys::shiftModifier;

       result.setInfo (String ("Beat Thang Track ") + track.toString(),
                       "One of the track buttons on a Beat Thang device",
                       "Beat Thang Hardware", 0);

       result.addDefaultKeypress (pressOffset + trackOffset, mkeys);

       return true;
   }

}}

#endif /* ELEMENT_GUI_COMMANDS_H */
