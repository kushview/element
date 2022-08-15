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

#include "JuceHeader.h"
#include "signals.hpp"

namespace element {

/** Monitors MIDI input/output from device IO in the audio engine */
class MidiIOMonitor : public ReferenceCountedObject
{
public:
    MidiIOMonitor() {}

    ~MidiIOMonitor()
    {
        midiSent.disconnect_all_slots();
        midiReceived.disconnect_all_slots();
    }

    Signal<void()> midiReceived;
    Signal<void()> midiSent;

    inline void clear()
    {
        midiInputCount.set (0);
        midiOutputCount.set (0);
    }

    inline void notify()
    {
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
        if (midiInputCount.get() > 0)
            midiReceived();
        if (midiOutputCount.get() > 0)
            midiSent();
        clear();
    }

    inline void received() { midiInputCount.set (midiInputCount.get() + 1); }
    inline void sent() { midiOutputCount.set (midiOutputCount.get() + 1); }

private:
    Atomic<int> midiInputCount { 0 };
    Atomic<int> midiOutputCount { 0 };
};

typedef ReferenceCountedObjectPtr<MidiIOMonitor> MidiIOMonitorPtr;

} // namespace element
