/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include <regex>
#include "JuceHeader.h"

namespace Element {

class OSCProcessor {
public:

    static bool isValidOscPort (int port)
    {
        return port > 0 && port < 65536;
    }

    static std::vector<std::string> parseOscAddressPaths(const OSCMessage& message)
    {
        /** AddressPattern.oscSymbols is already parsed, but it's a private member */
        const auto& addr = message.getAddressPattern().toString().toStdString();

        /** Parts: I/O Type, Device Name, Command */

        std::vector<std::string> paths;

        std::string delimiter = "/";
        std::string part = "";
        std::size_t currentPos = 0;
        std::size_t nextPos = 0;

        while (currentPos != std::string::npos)
        {
            currentPos = addr.find(delimiter, currentPos);
            nextPos = addr.find(delimiter, currentPos + 1);
            part = addr.substr(currentPos + 1, (nextPos - 1) - currentPos);
            currentPos = nextPos;
            if (part == "") continue;

            paths.push_back(part);
        }

        for (size_t i = paths.size(); i < 3; i++)
            paths.push_back("");

        return paths;
    }

    static var getOscArgumentAsPrimitiveValue(const OSCArgument& arg)
    {
        if (arg.isFloat32()) return arg.getFloat32();
        if (arg.isInt32())   return arg.getInt32();
        if (arg.isString())  return arg.getString();
        return 0;
    }

    static String getOSCArgumentAsString (const OSCArgument& arg)
    {
        String type;
        String value;

        if (arg.isFloat32())
        {
            type = "float32";
            value = String (arg.getFloat32());
        }
        else if (arg.isInt32())
        {
            type = "int32";
            value = String (arg.getInt32());
        }
        else if (arg.isString())
        {
            type = "string";
            value = arg.getString();
        }
        else if (arg.isBlob())
        {
            type = "blob";
            auto& blob = arg.getBlob();
            value = String::fromUTF8 ((const char*) blob.getData(), (int) blob.getSize());
        }
        else
        {
            type = "unknown";
            value = "value";
        }
        return type + " " + value;
    }

    static String getOSCMessageAsString (const OSCMessage& message)
    {
        OSCAddressPattern addressPattern = message.getAddressPattern();
        String str = addressPattern.toString();

        if ( message.isEmpty() ) return str;

        str += " ";
        int i = 0;
        for (auto& arg : message)
        {
            if (i > 0) str += ", ";
            str += getOSCArgumentAsString (arg);
            i++;
        }

        return str;
    }


    static MidiMessage processOscToMidiMessage(const OSCMessage& message)
    {

        std::vector<std::string> paths = parseOscAddressPaths(message);

        if (paths[0] != "midi")
        {
            return MidiMessage();
        }

        std::string deviceName;
        std::string command;

        /** /midi/{command} or /midi/{deviceName}/{command} */

        if (paths[2] == "")
        {
            deviceName = "";
            command = paths[1];
        }
        else if (paths[1] != "")
        {
            deviceName = paths[1];
            command = paths[2];
        }
        else
        {
            return MidiMessage();
        }

        /** Cast the rest of values to their respective types */

        int numArgs = message.size();
        var values[numArgs];
        int blobIndexInMessage = -1;

        int i = 0;
        for (auto& arg : message)
        {
            if (arg.isBlob())
            {
                blobIndexInMessage = i;
                values[i] = 0;
            }
            else
            {
                values[i] = getOscArgumentAsPrimitiveValue(arg);
            }
            i++;
        }

        /** Commands - See https://docs.juce.com/master/classMidiMessage.html#pub-static-methods */

        if (command == "raw")
        {
            if (blobIndexInMessage >= 0) {
                const MemoryBlock& blob = message[blobIndexInMessage].getBlob();
                // return String::fromUTF8 ((const char*) blob.getData(), (int) blob.getSize());
                return MidiMessage(blob.getData(), (int) blob.getSize());
            }
        }
        else if (command == "noteOn")
        {
            if (numArgs >= 3)
            {
                /** channel, noteNumber, velocity */
                return MidiMessage::noteOn((int) values[0], (int) values[1], (float) values[2]);
            }
        }
        else if (command == "noteOff")
        {
            if (numArgs >= 3)
            {
                /** channel, noteNumber, velocity */
                return MidiMessage::noteOff((int) values[0], (int) values[1], (float) values[2]);
            }
        }
        else if (command == "programChange")
        {
            if (numArgs >= 2)
            {
                /** channel, programNumber */
                return MidiMessage::noteOff((int) values[0], (int) values[1]);
            }
        }
        else if (command == "pitchBend")
        {
            if (numArgs >= 2)
            {
                /** channel, position */
                return MidiMessage::pitchWheel((int) values[0], (int) values[1]);
            }
        }
        else if (command == "afterTouch")
        {
            if (numArgs >= 2)
            {
                /** channel, noteNumber, aftertouchAmount */
                return MidiMessage::aftertouchChange((int) values[0], (int) values[1], (int) values[2]);
            }
        }
        else if (command == "channelPressure")
        {
            if (numArgs >= 2)
            {
                /** channel, pressure */
                return MidiMessage::noteOff((int) values[0], (int) values[1]);
            }
        }
        else if (command == "controlChange")
        {
            if (numArgs >= 3)
            {
                /** channel, controllerType, value */
                return MidiMessage::controllerEvent((int) values[0], (int) values[1], (int) values[2]);
            }
        }
        else if (command == "allNotesOff")
        {
            if (numArgs >= 1)
            {
                /** channel */
                return MidiMessage::allNotesOff((int) values[0]);
            }
        }
        else if (command == "allSoundOff")
        {
            if (numArgs >= 1)
            {
                /** channel */
                return MidiMessage::allSoundOff((int) values[0]);
            }
        }
        else if (command == "allControllersOff")
        {
            if (numArgs >= 1)
            {
                /** channel */
                return MidiMessage::allControllersOff((int) values[0]);
            }
        }
        else if (command == "start")
        {
            return MidiMessage::midiStart();
        }
        else if (command == "continue")
        {
            return MidiMessage::midiContinue();
        }
        else if (command == "stop")
        {
            return MidiMessage::midiStop();
        }
        else if (command == "clock")
        {
            return MidiMessage::midiClock();
        }
        else if (command == "songPositionPointer")
        {
            if (numArgs >= 1)
            {
                /** positionInMidiBeats */
                return MidiMessage::songPositionPointer((int) values[0]);
            }
        }
        else if (command == "activeSense")
        {
            return MidiMessage();
        }

        // Command "polyPressure" not supported?

        /** Unknown command */
        return MidiMessage();
    }

    static OSCMessage processMidiToOscMessage (const MidiMessage& m)
    {
        /** Address: /midi/{command} or /midi/{deviceName}/{command} */

        String path = "/midi/";

        const int channel = m.getChannel();

        if (m.isNoteOn())
        {
            return OSCMessage (path + "noteOn", channel, m.getNoteNumber(), m.getFloatVelocity());
        }
        else if (m.isNoteOff())
        {
            return OSCMessage (path + "noteOff", channel, m.getNoteNumber(), m.getFloatVelocity());
        }
        else if (m.isProgramChange())
        {
            return OSCMessage (path + "programChange", channel, m.getProgramChangeNumber());
        }
        else if (m.isPitchWheel())
        {
            return OSCMessage (path + "pitchBend", channel, m.getPitchWheelValue());
        }
        else if (m.isAftertouch())
        {
            return OSCMessage (path + "afterTouch", channel, m.getNoteNumber(), m.getAfterTouchValue());
        }
        else if (m.isChannelPressure())
        {
            return OSCMessage (path + "channelPressure", channel, m.getChannelPressureValue());
        }
        else if (m.isController())
        {
            return OSCMessage (path + "controlChange", channel, m.getControllerNumber(), m.getControllerValue());
        }
        else if (m.isAllNotesOff())
        {
            return OSCMessage (path + "allNotesOff");
        }
        else if (m.isAllSoundOff())
        {
            return OSCMessage (path + "allNotesOff");
        }
        else if (m.isResetAllControllers())
        {
            return OSCMessage (path + "allControllersOff");
        }
        else if (m.isMidiStart())
        {
            return OSCMessage (path + "start");
        }
        else if (m.isMidiContinue())
        {
            return OSCMessage (path + "continue");
        }
        else if (m.isMidiStop())
        {
            return OSCMessage (path + "stop");
        }
        else if (m.isMidiClock())
        {
            return OSCMessage (path + "clock");
        }
        else if (m.isSongPositionPointer ())
        {
            return OSCMessage (path + "songPositionPointer", m.getSongPositionPointerMidiBeat());
        }
        else if (m.isActiveSense ())
        {
            return OSCMessage (path + "activeSense");
        }

        return OSCMessage (path + "unknown");
    }


};

}
