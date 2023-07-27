// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/transport.hpp>
#include "tempo.hpp"

using namespace juce;

namespace element {

Transport::Transport()
    : playState (false),
      recordState (false)
{
    monitor = new Monitor();
    monitor->tempo.set (getTempo());

    seekWanted.set (false);
    seekFrame.set (0);

    nextBeatsPerBar.set (getBeatsPerBar());
    nextBeatDivisor.set (getBeatType());

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

void Transport::postProcess (int nframes)
{
    if (getTempo() != nextTempo.get())
    {
        setTempo (nextTempo.get());
        nextTempo.set (getTempo());
        monitor->tempo.set (nextTempo.get());
    }

    monitor->playing.set (playing);
    monitor->recording.set (recording);
    monitor->positionFrames.set (getPositionFrames());

    bool updateTimeScale = false;
    if (getBeatsPerBar() != nextBeatsPerBar.get())
    {
        ts.setBeatsPerBar ((unsigned short) nextBeatsPerBar.get());
        monitor->beatsPerBar.set (getBeatsPerBar());
        updateTimeScale = true;
    }

    if (ts.beatDivisor() != nextBeatDivisor.get())
    {
        ts.setBeatDivisor ((unsigned short) nextBeatDivisor.get());
        monitor->beatDivisor.set (nextBeatDivisor.get());
        updateTimeScale = true;
    }

    if (updateTimeScale)
        ts.updateScale();

    if (seekWanted.get())
    {
        if (getPositionFrames() != seekFrame.get())
            seekAudioFrame (seekFrame.get());
        seekWanted.set (false);
    }
}

void Transport::requestMeter (int beatsPerBar, int beatDivisor)
{
    if (beatsPerBar < 1)
        beatsPerBar = 1;
    if (beatsPerBar > 99)
        beatsPerBar = 99;
    if (beatDivisor < 0)
        beatDivisor = 0;
    if (beatDivisor > BeatType::SixteenthNote)
        beatDivisor = BeatType::SixteenthNote;
    nextBeatsPerBar.set (beatsPerBar);
    nextBeatDivisor.set (beatDivisor);
}

void Transport::requestAudioFrame (const int64_t frame)
{
    seekFrame.set (frame);
    seekWanted.set (true);
}

} // namespace element
