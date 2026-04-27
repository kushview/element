// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "element/juce.hpp"

#if JUCE_MAC
#include <CoreMIDI/CoreMIDI.h>
//#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>
#endif

#include "midioutput.h"
#include <memory>

namespace element {

class ElementMidiOutput
{
public:
    ElementMidiOutput() = delete;
    ElementMidiOutput (const juce::MidiDeviceInfo& deviceInfo);

    void closeDevice();
    static juce::Result openDevice (const juce::MidiDeviceInfo& deviceInfo, std::unique_ptr<ElementMidiOutput>& out);
    juce::Result sendBlockOfMessages (const juce::MidiBuffer& midi, double delayMs, double sampleRate) const;

private:
#if JUCE_MAC
    MIDIClientRef client {};
    MIDIEndpointRef destination {};
    MIDIPortRef port {};
    mach_timebase_info_data_t timebase {};
    MIDITimeStamp futureTimestamp (uint64_t nanosFromNow) const;
#else
    std::unique_ptr<juce::MidiOutput> output;
#endif
};

} // namespace element