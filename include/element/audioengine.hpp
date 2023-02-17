/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include <element/devicemanager.hpp>
#include <element/engine.hpp>
#include <element/midiiomonitor.hpp>
#include <element/runmode.hpp>
#include <element/session.hpp>
#include <element/transport.hpp>

namespace element {

class ClipFactory;
class EngineControl;
class Context;
class Settings;
class RootGraph;

class AudioEngine : public Engine {
public:
    Signal<void()> sampleLatencyChanged;
    AudioEngine (Context&, RunMode mode = RunMode::Standalone);
    virtual ~AudioEngine() noexcept;

    //==========================================================================
    RunMode getRunMode() const { return runMode; }

    //==========================================================================
    void activate();
    void deactivate();

    /** Adds a message to the MIDI input.  This can be used by Controllers and UI
        components that send MIDI in a non-realtime critical situation. DO NOT call
        this from the audio thread 
     
        @param msg                      The MidiMessage to send
        @param handleOnDeviceQueue      When true will treat it as if received by a
                                        MidiInputDevice callback (don't use except for debugging)
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
    AudioIODeviceCallback& getAudioIODeviceCallback() override;
    MidiInputCallback& getMidiInputCallback() override;

    /** For use by external systems only! e.g. the AU/VST version of Element and
        possibly things like rendering in the future
     */
    void prepareExternalPlayback (const double sampleRate, const int blockSize, const int numIns, const int numOuts);
    void processExternalBuffers (AudioBuffer<float>& buffer, MidiBuffer& midi);
    void processExternalPlayhead (AudioPlayHead* playhead, const int nframes);
    void releaseExternalResources();
    void updateExternalLatencySamples();
    int getExternalLatencySamples() const;

    Context& getWorld() const;
    MidiIOMonitorPtr getMidiIOMonitor() const;

private:
    class Private;
    std::unique_ptr<Private> priv;
    Context& world;
    RunMode runMode;
};

typedef ReferenceCountedObjectPtr<AudioEngine> AudioEnginePtr;

} // namespace element
