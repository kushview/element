// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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
