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

#include <cstdint>

#include <element/atomic.hpp>
#include <element/shuttle.hpp>

namespace element {

class Transport : public Shuttle {
public:
    class Monitor : public juce::ReferenceCountedObject {
    public:
        Monitor()
        {
            sampleRate.set (44100.0);
            beatsPerBar.set (4);
            beatType.set (2);
            beatDivisor.set (2);
        }

        juce::Atomic<int> beatsPerBar;
        juce::Atomic<int> beatType;
        juce::Atomic<int> beatDivisor;
        juce::Atomic<double> sampleRate;
        juce::Atomic<float> tempo;
        juce::Atomic<bool> playing;
        juce::Atomic<bool> recording;
        juce::Atomic<int64_t> positionFrames;

        inline double getPositionSeconds() const
        {
            return (double) positionFrames.get() / sampleRate.get();
        }

        inline float getPositionBeats() const
        {
            float numerator = (float) (1 << beatDivisor.get());
            float divisor = (float) (1 << beatType.get());
            divisor = divisor / numerator;
            divisor *= 60.f;
            return getPositionSeconds() * (tempo.get() / divisor);
        }

        inline void getBarsAndBeats (int& bars, int& beats, int& subBeats, int subDivisions = 4)
        {
            float t = getPositionBeats();
            bars = juce::roundToInt (std::floor (t / beatsPerBar.get()));
            beats = juce::roundToInt (std::floor (t)) % beatsPerBar.get();
            subBeats = juce::roundToInt (std::floor (t * subDivisions)) % subDivisions;
        }
    };

    typedef juce::ReferenceCountedObjectPtr<Monitor> MonitorPtr;

    Transport();
    ~Transport();

    int getBeatsPerBar() const { return getTimeScale().beatsPerBar(); }
    int getBeatType() const { return getTimeScale().beatType(); }

    inline void requestPlayState (bool p)
    {
        while (! playState.set (p)) {
        }
    }
    inline void requestPlayPause() { requestPlayState (! playState.get()); }
    inline void requestRecordState (bool r)
    {
        while (! recordState.set (r)) {
        }
    }
    inline void requestTempo (const double bpm)
    {
        while (! nextTempo.set (bpm)) {
        }
    }
    void requestMeter (int beatsPerBar, int beatType);

    void requestAudioFrame (const int64_t frame);

    void preProcess (int nframes);
    void postProcess (int nframes);

    inline MonitorPtr getMonitor() const { return monitor; }

private:
    AtomicValue<bool> playState, recordState;
    AtomicValue<double> nextTempo;
    juce::Atomic<int> nextBeatsPerBar, nextBeatDivisor;
    juce::Atomic<bool> seekWanted;
    AtomicValue<int64_t> seekFrame;
    MonitorPtr monitor;
};

} // namespace element