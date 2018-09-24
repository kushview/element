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
    const Identifier graph              = "graph";
    const Identifier graphs             = "graphs";
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
}

struct Alert
{
    inline static void showProductLockedAlert (const String& msg = String(), const String& title = "Feature not Available")
    {
        String message = (msg.isEmpty()) ? "Unlock the full version of Element to use this feature.\nGet a copy @ https://kushview.net"
                                        : msg;
        if (AlertWindow::showOkCancelBox (AlertWindow::InfoIcon, title, message, "Upgrade", "Cancel"))
            URL("https://kushview.net/products/element/").launchInDefaultBrowser();
    }
};

inline static void traceMidi (MidiBuffer& buf)
{
    MidiBuffer::Iterator iter (buf);
    MidiMessage msg; int frame = 0;
    
    while (iter.getNextEvent (msg, frame))
    {
        if (msg.isMidiClock())
        {
            DBG("clock:");
        }
        if (msg.isNoteOn())
        {
            DBG("NOTE ON");
            
        }
        if (msg.isNoteOff())
        {
            DBG("NOTE OFF");
        }
        
        if (msg.isAllNotesOff() || msg.isAllSoundOff()) {
            DBG("got it: " << frame);
        }
    }
}

}
