/*
    Signals.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
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
    const Identifier node               = "node";
    const Identifier nodes              = "nodes";
    const Identifier notes              = "notes";
    const Identifier persistent         = "persistent";
    const Identifier port               = "port";
    const Identifier ports              = "ports";
    
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
    
    const Identifier beatsPerBar        = "beatsPerBar";
    const Identifier beatDivisor        = "beatDivisor";
    const Identifier midiChannel        = "midiChannel";
    const Identifier renderMode         = "renderMode";
}

}
