/*
    Transport.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

namespace Element
{
    class Transport : public Shuttle
    {
    public:
        Transport();
        ~Transport();

        inline void requestPlayState (bool p) { while (! playState.set (p)) {} }
        inline void requestRecordState (bool r) { while (! recordState.set (r)) {} }
        inline void requestTempo (const double bpm) { while (! nextTempo.set (bpm)) {} }

        void preProcess (int nframes);
        void postProcess (int nframes);

        Shared<Monitor> monitor();

    private:
        AtomicValue<bool> playState, recordState;
        AtomicValue<double> nextTempo;
        Shared<Monitor> playPos;
    };
}
