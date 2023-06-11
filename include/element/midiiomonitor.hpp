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

#include <element/juce/core.hpp>
#include <element/signals.hpp>

namespace element {

/** Monitors MIDI input/output from device IO in the audio engine */
class MidiIOMonitor : public juce::ReferenceCountedObject {
public:
    MidiIOMonitor() {}

    ~MidiIOMonitor()
    {
        sigReceived.disconnect_all_slots();
        sigSent.disconnect_all_slots();
    }

    Signal<void()> sigReceived;
    Signal<void()> sigSent;

    inline void clear()
    {
        midiInputCount.set (0);
        midiOutputCount.set (0);
    }

    /** Call this from the UI thread regularly. */
    inline void notify()
    {
        if (midiInputCount.get() > 0)
            sigReceived();
        if (midiOutputCount.get() > 0)
            sigSent();
        clear();
    }

    /** Call in the midi thread when received. */
    inline void received() { midiInputCount.set (midiInputCount.get() + 1); }

    /** Call in the midi thread when sent. */
    inline void sent() { midiOutputCount.set (midiOutputCount.get() + 1); }

private:
    juce::Atomic<int> midiInputCount { 0 };
    juce::Atomic<int> midiOutputCount { 0 };
};

typedef juce::ReferenceCountedObjectPtr<MidiIOMonitor> MidiIOMonitorPtr;

} // namespace element
