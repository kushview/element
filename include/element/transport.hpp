// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>

#include <element/atomic.hpp>
#include <element/shuttle.hpp>

namespace element {

/** The actions offered by the transport bar buttons. Shared by the UI and by
    MIDI mappings so both drive the transport through the same rules. */
enum class TransportAction {
    Play,
    Stop,
    Record,
    SeekZero
};

/** Returns the persistent identifier of an action ("play", "stop", "record",
    "seekZero"), as stored in MIDI mappings. */
juce::String toString (TransportAction action);

/** Parses a persistent action identifier.
    @return The action, or nullopt if the string is not a known identifier. */
std::optional<TransportAction> transportActionFromString (const juce::String& id);

/** Returns the human readable name of an action ("Play", "Seek Start", ...). */
juce::String getTransportActionName (TransportAction action);

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

        /** Beats per bar (the time signature numerator, e.g. 4 for 4/4). */
        juce::Atomic<int> beatsPerBar;

        /** Beat type: the time signature denominator as a BeatType enum index
            (0 = whole, 1 = half, 2 = quarter, 3 = eighth, 4 = sixteenth). This
            is the indexed form of beatDivisor; see the BeatType struct. */
        juce::Atomic<int> beatType;

        /** Beat divisor: the time signature denominator itself (2, 4, 8, 16 …),
            i.e. the note value that gets one beat. beatType is the equivalent
            BeatType enum index of this value. */
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

        /** Returns the length of one beat relative to a quarter note, i.e.
            beatDivisor / 4 (whole = 0.25, quarter = 1.0, sixteenth = 4.0). */
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
    int getBeatsPerBar() const { return beatsPerBar; }

    /** Returns the beat type (time signature denominator as a BeatType enum
        index) from the current time signature. */
    int getBeatType() const { return beatType; }

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

    /** Performs a transport bar action (thread-safe).

        Decided on the requested state rather than the applied state, so rapid
        repeats behave as the buttons do: Play restarts from the beginning when
        already playing, Stop rewinds when already stopped, Record toggles and
        SeekZero rewinds.

        @param action The action to perform
    */
    void requestAction (TransportAction action);

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
        @param beatType    Beat type enum
    */
    void requestMeter (int beatsPerBar, int beatType);

    /** Requests a seek to a specific audio frame (thread-safe). The most
        recent request before the next audio block wins.
        @param frame Target position in samples; negative values seek to 0
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
    juce::Atomic<int> nextBeatsPerBar, nextBeatType;

    // Pending seek target, or noSeekRequested. A single atomic keeps the
    // "wanted" flag and the frame together, so concurrent requesters can never
    // leave a stale frame behind a set flag; the last request wins.
    static constexpr int64_t noSeekRequested = -1;
    std::atomic<int64_t> seekRequest { noSeekRequested };
    static_assert (std::atomic<int64_t>::is_always_lock_free, "seek requests must be lock-free");

    MonitorPtr monitor;
};

} // namespace element
