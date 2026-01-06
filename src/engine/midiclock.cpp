// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "engine/midiclock.hpp"

#include <cmath>

namespace element {

void MidiClock::process (const MidiMessage& msg)
{
    jassert (sampleRate > 0.0 && blockSize > 0);
    jassert (msg.isMidiClock() || msg.isSongPositionPointer());

    const double timestamp = msg.getTimeStamp();

    if (midiClockTicks <= 0)
    {
        // Initialize the DLL with the first MIDI clock message.
        // Expected period at 120 BPM: 60/(120*24) = 0.02083 seconds per clock
        // We use a generic initial period estimate and let the DLL converge.
        const double initialPeriodEstimate = 60.0 / (120.0 * 24.0);
        dll.reset (timestamp, initialPeriodEstimate, 24.0);
    }
    else
    {
        dll.update (timestamp);
    }

    // Notify listeners when signal is acquired
    if (midiClockTicks == syncPeriodTicks)
    {
        for (auto* listener : listeners)
            listener->midiClockSignalAcquired();
    }

    // Update BPM more frequently for responsive sync
    if (midiClockTicks >= syncPeriodTicks)
    {
        const double timeSinceUpdate = timestamp - timeOfLastUpdate;
        if (timeSinceUpdate >= bpmUpdateInterval)
        {
            lastKnownTimeDiff = dll.timeDiff();
            // MIDI clock has 24 pulses per quarter note (PPQN)
            const double bpm = 60.0 / (lastKnownTimeDiff * 24.0);

            // Only update if BPM is in valid range and has changed significantly
            if (bpm >= 20.0 && bpm <= 999.0)
            {
                const double bpmDiff = std::abs (bpm - lastReportedBpm);
                if (bpmDiff >= bpmChangeThreshold || lastReportedBpm == 0.0)
                {
                    lastReportedBpm = bpm;
                    for (auto* listener : listeners)
                        listener->midiClockTempoChanged (static_cast<float> (bpm));
                }
            }

            timeOfLastUpdate = timestamp;
        }
    }

    ++midiClockTicks;
}

void MidiClock::reset (const double sr, const int bs)
{
    sampleRate = sr;
    blockSize = bs;
    timeOfLastUpdate = 0.0;
    lastKnownTimeDiff = 0.0;
    lastReportedBpm = 0.0;
    midiClockTicks = 0;
}

void MidiClock::addListener (Listener* listener)
{
    if (listener)
        listeners.addIfNotAlreadyThere (listener);
}

void MidiClock::removeListener (Listener* listener)
{
    if (listener)
        listeners.removeFirstMatchingValue (listener);
}

} // namespace element
