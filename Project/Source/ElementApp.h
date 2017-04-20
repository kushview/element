/*
    Signals.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.
*/

#ifndef ELEMENT_APP_H
#define ELEMENT_APP_H

#if HAVE_JUCE_CORE
 #include <juce/juce.h>
 #include <element/element.h>
#else
 #include "JuceHeader.h"
#endif

namespace Element {
namespace Tags {
    const Identifier graph = "graph";
    const Identifier node = "node";
}
}

#endif  // ELEMENT_APP_H
