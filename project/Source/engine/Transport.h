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
        class Monitor : public ReferenceCountedObject {
        public:
            Atomic<float> tempo;
        };
        
        typedef ReferenceCountedObjectPtr<Monitor> MonitorPtr;
        
        Transport();
        ~Transport();

        inline void requestPlayState (bool p) { while (! playState.set (p)) { } }
        inline void requestRecordState (bool r) { while (! recordState.set (r)) { } }
        inline void requestTempo (const double bpm) { while (! nextTempo.set (bpm)) { } }

        void preProcess (int nframes);
        void postProcess (int nframes);

        inline MonitorPtr getMonitor() const { return monitor; }

    private:
        AtomicValue<bool> playState, recordState;
        AtomicValue<double> nextTempo;
        MonitorPtr monitor;
    };
}
