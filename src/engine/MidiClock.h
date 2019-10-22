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

#include "ElementApp.h"

namespace Element {
    
class MidiClock
{
public:
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() { }
        
        virtual void midiClockSignalAcquired() =0;
        virtual void midiClockSignalDropped() =0;
        virtual void midiClockTempoChanged (const float bpm) =0;
    };
    
    MidiClock() = default;
    ~MidiClock() { }
    
    void process (const MidiMessage& msg);
    void reset (const double sampleRate, const int blockSize);
    
    void addListener (Listener*);
    void removeListener (Listener*);
    
private:
    double sampleRate = 0.0;
    int blockSize = 0;
    DelayLockedLoop dll;
    double timeOfLastUpdate = 0.0;
    double lastKnownTimeDiff = 0.0;
    int midiClockTicks = 0;
    int syncPeriodTicks = 48;
    double bpmUpdateSeconds = 1.0;
    
    Array<Listener*> listeners;
};

class MidiClockMaster
{
public:
    MidiClockMaster()
    {
        clockMessage = MidiMessage::midiClock();
        updateCoefficients();
    }

    ~MidiClockMaster() noexcept { }

    inline void reset()
    {
        pos = 0;
        updateCoefficients();
    }

    inline void setTempo (const double newTempo) noexcept
    {
        if (tempo == newTempo)
            return;
        tempo = newTempo;
        updateCoefficients();
    }

    inline void setSampleRate (const double newSampleRate) noexcept
    {
        if (sampleRate == newSampleRate)
            return;
        sampleRate = newSampleRate;
        updateCoefficients();
    }

    inline void render (MidiBuffer& midi, int numSamples) noexcept
    {
        if (samplesPerClock <= 0)
            return;

        int frame = static_cast<int> (pos % samplesPerClock);
        if (frame > 0) frame = samplesPerClock - frame;
        while (frame < numSamples)
        {
            midi.addEvent (clockMessage, frame);
            frame += samplesPerClock;
        }

        pos += numSamples;
    }

private:
    MidiMessage clockMessage;
    int64 pos = 0;
    double tempo = 120.0;
    double sampleRate = 44100.0;
    int samplesPerClock = 0;
    double clocksPerMinute = 1.0;

    void updateCoefficients()
    {
        clocksPerMinute = 24.f * tempo;
        samplesPerClock = roundToIntAccurate ((60.0 * sampleRate) / clocksPerMinute);
    }
};

}
