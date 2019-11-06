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

const static std::regex oscAddressWithArity1 { "/[^/\\n\\r]+$" };
const static std::regex oscAddressWithArity2 { "/([^/\\n\\r]+)/([^/\\n\\r]+)" };
const static std::regex oscAddressWithArity3 { "/([^/\\n\\r]+)/([^/\\n\\r]+)/([^/\\n\\r]+)" };

class OscProcessor {
public:

    static var castOscArgumentPrimitiveValue(const OSCArgument& arg)
    {
        if (arg.isFloat32())
        {
            return arg.getFloat32();
        }
        if (arg.isInt32())
        {
            return arg.getInt32();
        }
        if (arg.isString())
        {
            return arg.getString();
        }
        return 0;
    }

    static std::vector<std::string> extractOscAddressParts(const OSCMessage& message)
    {

        const auto& addressString = message.getAddressPattern().toString().toStdString();

        int arity = 0;
        int maxArity = 3;

        std::smatch match;

        /** I/O Type, Device Name, Command */
        std::vector<std::string> parts;

        if (std::regex_match(addressString, match, oscAddressWithArity1))
        {
            arity = 1;
        }
        else if (std::regex_match(addressString, match, oscAddressWithArity2))
        {
            arity = 2;
        }
        else if (std::regex_match(addressString, match, oscAddressWithArity3))
        {
            arity = 3;
        }

        /** Skip first element in smatch, it's the whole match */
        for (int i = 1; i <= maxArity; i++)
        {
            if (arity >= i)
            {
                parts.push_back(match.str(i));
            }
            else {
                parts.push_back("");
            }
        }

        return parts;
    }

    static MidiMessage processOscToMidiMessage(const OSCMessage& message)
    {

        std::vector<std::string> addressParts = extractOscAddressParts(message);

        if (addressParts[0] != "midi")
        {
            return MidiMessage();
        }

        std::string deviceName;
        std::string command;

        /** /midi/{command} or /midi/{deviceName}/{command} */

        if (addressParts[2] == "")
        {
            deviceName = "";
            command = addressParts[1];
        }
        else if (addressParts[1] != "")
        {
            deviceName = addressParts[1];
            command = addressParts[2];
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
                values[i] = castOscArgumentPrimitiveValue(arg);
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

};

}
