/*
    Transport.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "Transport.h"

namespace Element {


    Transport::Transport()
        : playState (false),
          recordState (false)
    {
        Array<int> a;
        playPos.reset (new Monitor (a, 0));
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
            playPos->set (getPositionBeats());
        }
    }

    Shared<Monitor> Transport::monitor()
    {
        return playPos;
    }

    void Transport::postProcess (int nframes)
    {
        if (getTempo() != nextTempo.get())
        {
            DBG("[EL] tempo change in transport: " << getTempo() << " -> " << nextTempo.get());
            setTempo (nextTempo.get());
            nextTempo.set (getTempo());
        }
    }
}
