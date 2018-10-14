/*
    ElementApp.h - This file is part of Element
    Copyright (C) 2014-2018  Kushview, LLC.  All rights reserved.
 */

#pragma once

#if HAVE_JUCE_CORE
 #include <juce/juce.h>
 #include <element/element.h>
#else
 #include "JuceHeader.h"
#endif

#include "DataPath.h"
namespace kv { }
using namespace kv;

#if EL_RUNNING_AS_PLUGIN
 #include "../project/JuceLibraryCode/BinaryData.h"
#endif

namespace Element {
namespace Tags
{
    using namespace kv::Slugs;
    const Identifier active             = "active";
    const Identifier arc                = "arc";
    const Identifier arcs               = "arcs";
    const Identifier bypass             = "bypass";
    const Identifier control            = "control";
    const Identifier controller         = "controller";
    const Identifier controllers        = "controllers";
    const Identifier graph              = "graph";
    const Identifier graphs             = "graphs";
    const Identifier mappingData        = "mappingData";
    const Identifier map                = "map";
    const Identifier maps               = "maps";
    const Identifier missing            = "missing";
    const Identifier node               = "node";
    const Identifier nodes              = "nodes";
    const Identifier notes              = "notes";
    const Identifier persistent         = "persistent";
    const Identifier placeholder        = "placeholder";
    const Identifier port               = "port";
    const Identifier ports              = "ports";
    const Identifier preset             = "preset";
	const Identifier program			= "program";
    const Identifier sourceNode         = "sourceNode";
    const Identifier sourcePort         = "sourcePort";
    const Identifier sourceChannel      = "sourceChannel";
    const Identifier destNode           = "destNode";
    const Identifier destPort           = "destPort";
    const Identifier destChannel        = "destChannel";
    
    const Identifier identifier         = "identifier";
    const Identifier format             = "format";
    const Identifier flow               = "flow";
    const Identifier input              = "input";
    const Identifier object             = "object";
    const Identifier output             = "output";
    
    const Identifier session            = "session";
    const Identifier state              = "state";
	const Identifier programState		= "programState";
    const Identifier beatsPerBar        = "beatsPerBar";
    const Identifier beatDivisor        = "beatDivisor";
    const Identifier midiChannel        = "midiChannel";
    const Identifier midiProgram        = "midiProgram";
    const Identifier renderMode         = "renderMode";

    const Identifier vertical           = "vertical";
    const Identifier staticPos          = "staticPos";

    const Identifier plugin             = "plugin";

    const Identifier windowOnTop        = "windowOnTop";
    const Identifier windowVisible      = "windowVisible";
    const Identifier windowX            = "windowX";
    const Identifier windowY            = "windowY";       

    const Identifier uuid               = "uuid";
    const Identifier parameter          = "parameter";
}

struct Alert
{
    inline static String productLockedMessage (const String& msg = String())
    {
        String message = (msg.isEmpty()) ? "Unlock the full version of Element to use this feature." : msg;
        message << "\nGet a copy @ https://kushview.net,"
                << "\n\nor enter your license key in preferences.";
        return message;
    }

    inline static void showProductLockedAlert (const String& msg = String(), const String& title = "Feature not Available")
    {
        const auto message = productLockedMessage (msg);
        if (AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, title, message, "Upgrade", "Cancel"))
            URL("https://kushview.net/products/element/").launchInDefaultBrowser();
        
    }

    inline static void showProductLockedAlertAsync (const String& msg = String(), 
                                                    const String& title = "Feature not Available")
    {
        const auto message = productLockedMessage (msg);
        AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon, title, message, "Ok");
    }
};

inline static void traceMidi (const MidiMessage& msg, const int frame = -1)
{
    if (msg.isMidiClock())
    {
        DBG("clock:");
    }
    if (msg.isNoteOn())
    {
        DBG("NOTE ON: " << msg.getMidiNoteName (msg.getNoteNumber(), true, false, 4));
    }
    if (msg.isNoteOff())
    {
        DBG("NOTE OFF: " << msg.getMidiNoteName (msg.getNoteNumber(), true, false, 4));
    }
    
    if (msg.isController())
    {
        msg.isControllerOfType (0);
        DBG("MIDI CC: " << msg.getControllerNumber() << ":" << msg.getControllerValue());
    }

    if (frame >= 0 && (msg.isAllNotesOff() || msg.isAllSoundOff()))
        DBG("got it: " << frame);
}

inline static void traceMidi (MidiBuffer& buf)
{
    MidiBuffer::Iterator iter (buf);
    MidiMessage msg; int frame = 0;
    while (iter.getNextEvent (msg, frame))
        traceMidi (msg, frame);
}

}
