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

#include "JuceHeader.h"
#include "DataPath.h"
#include "Signals.h"

namespace kv {
}
using namespace kv;

//=============================================================================
#ifdef __cplusplus
#define EL_EXTERN extern "C"
#else
#define EL_EXTERN
#endif

#ifdef _WIN32
#define EL_EXPORT EL_EXTERN __declspec(dllexport)
#else
#define EL_EXPORT EL_EXTERN __attribute__ ((visibility ("default")))
#endif

//=============================================================================
namespace Element {

using PortType = kv::PortType;

namespace Tags {
    using namespace kv::Slugs;
    const Identifier active = "active";
    const Identifier arc = "arc";
    const Identifier arcs = "arcs";
    const Identifier bypass = "bypass";
    const Identifier control = "control";
    const Identifier controller = "controller";
    const Identifier controllers = "controllers";
    const Identifier collapsed = "collapsed";
    const Identifier delayCompensation = "delayCompensation";
    const Identifier enabled = "enabled";
    const Identifier gain = "gain";
    const Identifier graph = "graph";
    const Identifier graphs = "graphs";
    const Identifier mappingData = "mappingData";
    const Identifier map = "map";
    const Identifier maps = "maps";
    const Identifier missing = "missing";
    const Identifier mute = "mute";
    const Identifier node = "node";
    const Identifier nodes = "nodes";
    const Identifier notes = "notes";
    const Identifier oversamplingFactor = "oversamplingFactor";
    const Identifier persistent = "persistent";
    const Identifier placeholder = "placeholder";
    const Identifier port = "port";
    const Identifier ports = "ports";
    const Identifier preset = "preset";
    const Identifier program = "program";
    const Identifier sourceNode = "sourceNode";
    const Identifier sourcePort = "sourcePort";
    const Identifier sourceChannel = "sourceChannel";
    const Identifier destNode = "destNode";
    const Identifier destPort = "destPort";
    const Identifier destChannel = "destChannel";

    const Identifier identifier = "identifier";
    const Identifier symbol = "symbol";
    const Identifier format = "format";
    const Identifier flow = "flow";
    const Identifier input = "input";
    const Identifier object = "object";
    const Identifier output = "output";

    const Identifier session = "session";
    const Identifier state = "state";
    const Identifier programState = "programState";
    const Identifier beatsPerBar = "beatsPerBar";
    const Identifier beatDivisor = "beatDivisor";
    const Identifier midiChannel = "midiChannel";
    const Identifier midiChannels = "midiChannels";
    const Identifier midiProgram = "midiProgram";
    const Identifier midiProgramsEnabled = "midiProgramsEnabled";
    const Identifier globalMidiPrograms = "globalMidiPrograms";
    const Identifier midiProgramsState = "midiProgramsState";
    const Identifier renderMode = "renderMode";

    const Identifier vertical = "vertical";
    const Identifier staticPos = "staticPos";

    const Identifier plugin = "plugin";

    const Identifier windowOnTop = "windowOnTop";
    const Identifier windowVisible = "windowVisible";
    const Identifier x = "x";
    const Identifier y = "y";
    const Identifier windowX = "windowX";
    const Identifier windowY = "windowY";
    const Identifier relativeX = "relativeX";
    const Identifier relativeY = "relativeY";
    const Identifier width = "width";
    const Identifier height = "height";
    const Identifier pluginName = "pluginName";
    const Identifier pluginIdentifierString = "pluginIdentifierString";
    const Identifier uuid = "uuid";
    const Identifier ui = "ui";
    const Identifier parameter = "parameter";
    const Identifier offline = "offline";

    const Identifier transpose = "transpose";
    const Identifier keyStart = "keyStart";
    const Identifier keyEnd = "keyEnd";

    const Identifier velocityCurveMode = "velocityCurveMode";
    const Identifier workspace = "workspace";

    const Identifier externalSync = "externalSync";

    const Identifier updater = "updater";
} // namespace Tags

struct Alert
{
    inline static String productLockedMessage (const String& msg = String())
    {
        String message = (msg.isEmpty()) ? "Unlock the full version of Element to use this feature." : msg;
        message << "\nGrab a copy at https://kushview.net, or enter your license key in preferences.";
        return message;
    }

    inline static void showProductLockedAlert (const String& msg = String(), const String& title = "Feature not Available")
    {
        const auto message = productLockedMessage (msg);
        if (AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, title, message, "Upgrade", "Cancel"))
            URL ("https://kushview.net/products/element/").launchInDefaultBrowser();
    }

    inline static void showProductLockedAlertAsync (const String& msg = String(),
                                                    const String& title = "Feature not Available")
    {
        const auto message = productLockedMessage (msg);
        class Callback : public ModalComponentManager::Callback
        {
        public:
            void modalStateFinished (int returnValue)
            {
                if (returnValue > 0)
                    URL ("https://kushview.net/products/element/").launchInDefaultBrowser();
            }
        };

        AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, title, message, "Upgrade", "Cancel", nullptr, new Callback());
    }
};

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

} // namespace Element
