// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include <element/atomic.hpp>
#include <element/shuttle.hpp>

namespace element {

/** Audio transport with thread-safe state management.
    
    Extends Shuttle to provide a complete transport implementation with
    lock-free communication between the audio thread and other threads.
    
    State changes (play, record, tempo, seek) are requested from any thread
    and applied during preProcess() on the audio thread. The Monitor class
    provides read-only access to current transport state for UI updates.
    
    @see Shuttle, Monitor
*/
class Transport : public Shuttle {
public:
    /** Thread-safe, read-only view of transport state.
        
        Monitor provides atomic access to transport parameters for UI display
        and other non-audio threads. Values are updated by the Transport
        during audio processing.
        
        Reference counted for safe sharing between threads.
    */
    class Monitor : public juce::ReferenceCountedObject {
    public:
        Monitor();

        /** Number of beats per bar (time signature numerator). */
        juce::Atomic<int> beatsPerBar;

        /** Beat type/unit (time signature denominator, e.g. 4 = quarter note). */
        juce::Atomic<int> beatType;

        /** Beat divisor for subdivision calculations. */
        juce::Atomic<int> beatDivisor;

        /** Current sample rate in Hz. */
        juce::Atomic<double> sampleRate;

        /** Current tempo in beats per minute. */
        juce::Atomic<float> tempo;

        /** True if transport is currently playing. */
        juce::Atomic<bool> playing;

        /** True if transport is currently recording. */
        juce::Atomic<bool> recording;

        /** Current playhead position in audio frames (samples). */
        juce::Atomic<int64_t> positionFrames;

        /** Returns the ratio of beat type to beat divisor. */
        double beatRatio() const noexcept;

        /** Returns the current position in seconds. */
        double getPositionSeconds() const;

        /** Returns the current position in beats. */
        float getPositionBeats() const;

        /** Calculates bars, beats, and sub-beats from current position.
            @param bars        Output: bar number (0-based)
            @param beats       Output: beat within bar (0-based)
            @param subBeats    Output: sub-beat within beat (0-based)
            @param subDivisions Number of sub-divisions per beat (default: 4)
        */
        void getBarsAndBeats (int& bars, int& beats, int& subBeats, int subDivisions = 4);
    };

    using MonitorPtr = juce::ReferenceCountedObjectPtr<Monitor>;

    Transport();
    ~Transport();

    /** Returns beats per bar from the current time signature. */
    int getBeatsPerBar() const { return getTimeScale().beatsPerBar(); }

    /** Returns beat type (denominator) from the current time signature. */
    int getBeatType() const { return getTimeScale().beatType(); }

    /** Requests a play state change (thread-safe).
        @param p True to play, false to stop
    */
    inline void requestPlayState (bool p)
    {
        while (! playState.set (p)) {
        }
    }

    /** Toggles between play and pause (thread-safe). */
    inline void requestPlayPause() { requestPlayState (! playState.get()); }

    /** Requests a record state change (thread-safe).
        @param r True to record, false to stop recording
    */
    inline void requestRecordState (bool r)
    {
        while (! recordState.set (r)) {
        }
    }

    /** Requests a tempo change (thread-safe).
        @param bpm New tempo in beats per minute
    */
    inline void requestTempo (const double bpm)
    {
        while (! nextTempo.set (bpm)) {
        }
    }

    /** Requests a time signature change (thread-safe).
        @param beatsPerBar Numerator of time signature
        @param beatType    Denominator of time signature
    */
    void requestMeter (int beatsPerBar, int beatType);

    /** Requests a seek to a specific audio frame (thread-safe).
        @param frame Target position in samples
    */
    void requestAudioFrame (const int64_t frame);

    /** Called at the start of each audio block to apply pending state changes.
        @param nframes Number of samples in the upcoming block
    */
    void preProcess (int nframes);

    /** Called at the end of each audio block to update the monitor.
        @param nframes Number of samples that were processed
    */
    void postProcess (int nframes);

    /** Returns the monitor for reading transport state from other threads. */
    inline MonitorPtr getMonitor() const { return monitor; }

    /** Applies any pending tempo change immediately.
        
        @warning Must only be called from the audio thread. Calling from other
        threads may cause race conditions. Safe to call when the audio engine 
        is not running.
    */
    void applyTempo() noexcept;

private:
    AtomicValue<bool> playState, recordState;
    AtomicValue<double> nextTempo;
    juce::Atomic<int> nextBeatsPerBar, nextBeatDivisor;
    juce::Atomic<bool> seekWanted;
    AtomicValue<int64_t> seekFrame;
    MonitorPtr monitor;
};

} // namespace element
