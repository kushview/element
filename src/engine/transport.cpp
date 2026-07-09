// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/transport.hpp>
#include "tempo.hpp"

using namespace juce;

namespace element {

Transport::Monitor::Monitor()
{
    sampleRate.set (44100.0);
    beatsPerBar.set (4);
    beatType.set (2);
    beatDivisor.set (4);
}

double Transport::Monitor::beatRatio() const noexcept
{
    switch (beatType.get())
    {
        case 0:
            return 0.25;
            break;
        case 1:
            return 0.5;
            break;
        case 2:
            return 1.0;
            break;
        case 3:
            return 2.0;
            break;
        case 4:
            return 4.0;
            break;
    }

    return 1.0;
}

double Transport::Monitor::getPositionSeconds() const
{
    return (double) positionFrames.get() / sampleRate.get();
}

float Transport::Monitor::getPositionBeats() const
{
    return getPositionSeconds() * (tempo.get() / 60.f);
}

void Transport::Monitor::getBarsAndBeats (int& bars, int& beats, int& subBeats, int subDivisions)
{
    float t = getPositionBeats();
    bars = juce::roundToInt (std::floor (t / beatsPerBar.get()));
    beats = juce::roundToInt (std::floor (t)) % beatsPerBar.get();
    subBeats = juce::roundToInt (std::floor (t * subDivisions)) % subDivisions;
}

Transport::Transport()
    : playState (false),
      recordState (false)
{
    monitor = new Monitor();
    monitor->tempo.set (getTempo());

    seekWanted.set (false);
    seekFrame.set (0);

    nextBeatsPerBar.set (getBeatsPerBar());
    nextBeatType.set (getBeatType());

    setLengthFrames (0);
}

Transport::~Transport() {}

void Transport::preProcess (int nframes)
{
    if (recording != recordState.get())
    {
        recording = recordState.get();
    }

    if (playing != playState.get())
    {
        playing = playState.get();
    }

    if (playing)
    {
    }
}

void Transport::applyTempo() noexcept
{
    if (getTempo() != nextTempo.get())
    {
        setTempo (nextTempo.get());
        nextTempo.set (getTempo());
        monitor->tempo.set (nextTempo.get());
    }
}

void Transport::postProcess (int nframes)
{
    applyTempo();

    monitor->playing.set (playing);
    monitor->recording.set (recording);
    monitor->positionFrames.set (getPositionFrames());

    if (getBeatsPerBar() != nextBeatsPerBar.get())
    {
        beatsPerBar = nextBeatsPerBar.get();
        monitor->beatsPerBar.set (getBeatsPerBar());
    }

    if (beatType != nextBeatType.get())
    {
        beatType = nextBeatType.get();
        // beatType is the BeatType enum index; beatDivisor is the raw
        // time signature denominator (1 << index): quarter (2) -> 4.
        beatDivisor = BeatType (static_cast<BeatType::ID> (beatType)).divisor();
        monitor->beatDivisor.set (beatDivisor);
        monitor->beatType.set (beatType);
    }

    if (seekWanted.get())
    {
        if (getPositionFrames() != seekFrame.get())
            seekAudioFrame (seekFrame.get());
        seekWanted.set (false);
    }
}

void Transport::requestMeter (int beatsPerBar, int beatType)
{
    if (beatsPerBar < 1)
        beatsPerBar = 1;
    if (beatsPerBar > 99)
        beatsPerBar = 99;
    if (beatType < 0)
        beatType = 0;
    if (beatType > BeatType::SixteenthNote)
        beatType = BeatType::SixteenthNote;
    nextBeatsPerBar.set (beatsPerBar);
    nextBeatType.set (beatType);
}

void Transport::requestAudioFrame (const int64_t frame)
{
    seekFrame.set (frame);
    seekWanted.set (true);
}

} // namespace element
