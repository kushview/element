
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

}
