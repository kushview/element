/*
    Transport.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "Transport.h"

namespace Element
{

Transport::Transport()
    : playState (false),
      recordState (false)
{
    monitor = new Monitor();
    monitor->tempo.set (getTempo());
    
    seekWanted.set(false);
    seekFrame.set (0);
    
    nextBeatsPerBar.set (getBeatsPerBar());
    nextBeatType.set (getBeatType());
    
    setLengthFrames (0);
}

Transport::~Transport() { }

void Transport::preProcess (int nframes)
{
    if (recording != recordState.get()) {
        recording = recordState.get();
    }

    if (playing != playState.get()) {
        playing = playState.get();
    }

    if (playing) {
        
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
    
    if (getBeatsPerBar() != nextBeatsPerBar.get() || getBeatType() != nextBeatType.get())
    {
        ts.setBeatType ((unsigned short) nextBeatType.get());
        ts.setBeatsPerBar ((unsigned short) nextBeatsPerBar.get());
        ts.updateScale();
        
        monitor->beatsPerBar.set (getBeatsPerBar());
        monitor->beatType.set (getBeatType());
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
    if (beatsPerBar < 1) beatsPerBar = 1;
    if (beatsPerBar > 99) beatsPerBar = 99;
    if (beatType < 0) beatType = 0;
    if (beatType > BeatType::SixteenthNote) beatType = BeatType::SixteenthNote;
    nextBeatsPerBar.set (beatsPerBar);
    nextBeatType.set (beatType);
}

void Transport::requestAudioFrame (const int64 frame)
{
    seekFrame.set (frame);
    seekWanted.set (true);
}

}
