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

#include "engine/MidiClock.h"

namespace Element {

void MidiClock::process (const MidiMessage& msg)
{
    jassert (sampleRate > 0.0 && blockSize > 0);
    jassert (msg.isMidiClock() || msg.isSongPositionPointer());

    if (midiClockTicks <= 0)
    {
        dll.reset (msg.getTimeStamp(), (double) blockSize / sampleRate, 1.0);
        dll.setParams ((double) blockSize / sampleRate, 1.0);
    }
    else
    {
        dll.update (msg.getTimeStamp());
    }

    if (midiClockTicks == syncPeriodTicks)
        for (auto* listener : listeners)
            listener->midiClockSignalAcquired();

    if (midiClockTicks >= syncPeriodTicks && msg.getTimeStamp() - timeOfLastUpdate >= 1.0)
    {
        lastKnownTimeDiff = dll.timeDiff();
        const double bpm = 60.0 / (lastKnownTimeDiff * 24.0);
        timeOfLastUpdate = msg.getTimeStamp();

        if (bpm >= 20.0 && bpm <= 999.0)
            for (auto* listener : listeners)
                listener->midiClockTempoChanged (bpm);
    }

    ++midiClockTicks;
}

void MidiClock::reset (const double sr, const int bs)
{
    sampleRate = sr;
    blockSize = bs;
    timeOfLastUpdate = 0.0;
    lastKnownTimeDiff = 0.0;
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

} // namespace Element
