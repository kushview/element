// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/audio_devices.hpp>

#include <element/midiiomonitor.hpp>
#include <element/runmode.hpp>
#include <element/session.hpp>
#include <element/transport.hpp>

namespace element {

class Context;
class Settings;
class RootGraph;

class AudioEngine final : public juce::ReferenceCountedObject {
public:
    Signal<void()> sampleLatencyChanged;
    AudioEngine (Context&, RunMode mode = RunMode::Standalone);
    virtual ~AudioEngine() noexcept;

    //==========================================================================
    RunMode getRunMode() const { return runMode; }

    //==========================================================================
    void activate();
    void deactivate();

    /** Adds a message to the MIDI input.  This can be used by Controllers and 
        UI components that send MIDI in a non-realtime critical situation. Do 
        not call this from the audio thread.
     
        @param msg                      The MidiMessage to send
        @param handleOnDeviceQueue      When true will treat it as if received 
                                        by a MidiInputDevice callback (don't use 
                                        except for debugging)
     */
    void addMidiMessage (const MidiMessage msg, bool handleOnDeviceQueue = false);

    void applySettings (Settings&);

    bool isUsingExternalClock() const;

    void setSession (SessionPtr);
    void refreshSession();

    bool addGraph (RootGraph* graph);
    bool removeGraph (RootGraph* graph);

    void setCurrentGraph (const int index) { setActiveGraph (index); }
    void setActiveGraph (const int index);
    int getActiveGraph() const;

    RootGraph* getGraph (const int index);

    void setPlaying (const bool shouldBePlaying);
    void setRecording (const bool shouldBeRecording);
    void seekToAudioFrame (const int64 frame);
    void setMeter (int beatsPerBar, int beatDivisor);

    void togglePlayPause();

    MidiKeyboardState& getKeyboardState();
    Transport::MonitorPtr getTransportMonitor() const;
    AudioIODeviceCallback& getAudioIODeviceCallback();
    MidiInputCallback& getMidiInputCallback();

    /** For use by external systems only! e.g. the AU/VST version of Element and
        possibly things like rendering in the future
     */
    void prepareExternalPlayback (const double sampleRate, const int blockSize, const int numIns, const int numOuts);
    void processExternalBuffers (AudioBuffer<float>& buffer, MidiBuffer& midi);
    void processExternalPlayhead (AudioPlayHead* playhead, const int nframes);
    void releaseExternalResources();
    void updateExternalLatencySamples();
    int getExternalLatencySamples() const;

    Context& context() const;
    MidiIOMonitorPtr getMidiIOMonitor() const;

    struct LevelMeter : public juce::ReferenceCountedObject {
        LevelMeter() noexcept {}
        inline double level() const noexcept { return _level.get(); }

    private:
        friend class AudioEngine;

        Atomic<float> _level { 0 };
        void updateLevel (const float* const*, int numChannels, int numSamples) noexcept;
    };

    using LevelMeterPtr = ReferenceCountedObjectPtr<LevelMeter>;

    LevelMeterPtr getLevelMeter (int channel, bool input);
    int getNumChannels (bool input) const noexcept;

private:
    class Private;
    std::unique_ptr<Private> priv;
    Context& world;
    RunMode runMode;
};

using AudioEnginePtr = ReferenceCountedObjectPtr<AudioEngine>;

} // namespace element
