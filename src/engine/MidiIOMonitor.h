#pragma once

#include "JuceHeader.h"
#include "Signals.h"

namespace Element {

/** Monitors MIDI input/output from device IO in the audio engine */
class MidiIOMonitor : public ReferenceCountedObject
{
public:
    MidiIOMonitor() { }

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
        jassert(MessageManager::getInstance()->isThisTheMessageThread());
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

} // namespace Element
