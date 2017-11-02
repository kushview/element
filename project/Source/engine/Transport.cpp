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
}

Transport::~Transport() { }

void Transport::preProcess (int nframes)
{
    if (recording != recordState.get()) {
        
    }

    if (playing != playState.get()) {
        
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
}

}
