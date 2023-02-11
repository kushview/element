/*
    This file is part of Element
    Copyright (C) 2014-2020  Kushview, LLC.  All rights reserved.

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

#include <JuceHeader.h>
#include "datapath.hpp"
#include <element/porttype.hpp>
#include <element/signals.hpp>

#include <element/element.hpp>
#include <element/tags.hpp>

namespace element {

inline static void traceMidi (const MidiMessage& msg, const int frame = -1)
{
    if (msg.isMidiClock())
    {
        DBG ("clock:");
    }
    if (msg.isNoteOn())
    {
        DBG ("NOTE ON: " << msg.getMidiNoteName (msg.getNoteNumber(), true, false, 4));
    }
    if (msg.isNoteOff())
    {
        DBG ("NOTE OFF: " << msg.getMidiNoteName (msg.getNoteNumber(), true, false, 4));
    }

    if (msg.isController())
    {
        msg.isControllerOfType (0);
        DBG ("MIDI CC: " << msg.getControllerNumber() << ":" << msg.getControllerValue());
    }

    if (msg.isProgramChange())
    {
        DBG ("program change: " << msg.getProgramChangeNumber() << " ch: " << msg.getChannel());
    }

    if (frame >= 0 && (msg.isAllNotesOff() || msg.isAllSoundOff()))
    {
        DBG ("got it: " << frame);
    }
}

inline static void traceMidi (MidiBuffer& buf)
{
    MidiBuffer::Iterator iter (buf);
    MidiMessage msg;
    int frame = 0;
    while (iter.getNextEvent (msg, frame))
        traceMidi (msg, frame);
}

inline static bool canConnectToWebsite (const URL& url, const int timeout = 2000)
{
    std::unique_ptr<InputStream> in (url.createInputStream (false, nullptr, nullptr, String(), timeout, nullptr));
    return in != nullptr;
}

inline static bool areMajorWebsitesAvailable()
{
    const char* urlsToTry[] = {
        "http://google.com", "http://bing.com", "http://amazon.com", "https://google.com", "https://bing.com", "https://amazon.com", nullptr
    };

    for (const char** url = urlsToTry; *url != nullptr; ++url)
        if (canConnectToWebsite (URL (*url)))
            return true;

    return false;
}

} // namespace element
