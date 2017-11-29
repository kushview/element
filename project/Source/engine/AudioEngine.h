/*
    AudioEngine.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "engine/Engine.h"
#include "engine/GraphProcessor.h"
#include "engine/Transport.h"
#include "session/DeviceManager.h"
#include "session/Session.h"

namespace Element {

class Globals;
class ClipFactory;
class EngineControl;
class Settings;

typedef GraphProcessor::AudioGraphIOProcessor IOProcessor;

class RootGraph : public GraphProcessor
{
public:
    RootGraph();
    ~RootGraph() { }
    
    void setValueTree (const ValueTree& tree);
    void setPlayConfigFor (AudioIODevice* device);
    void setPlayConfigFor (const DeviceManager::AudioDeviceSetup& setup);
    
    const String getName() const override;
    const String getInputChannelName (int channelIndex) const override;
    const String getOutputChannelName (int channelIndex) const override;

private:
    GraphNodePtr ioNodes [IOProcessor::numDeviceTypes];
    String graphName = "Device";
    StringArray audioInputNames;
    StringArray audioOutputNames;
    
    void updateChannelNames (AudioIODevice* device);
};

class AudioEngine : public Engine
{
public:
    AudioEngine (Globals&);
    ~AudioEngine();

    void activate();
    void deactivate();
    
    /** Adds a message to the MIDI input.  This can be used by Controllers and UI
        components that send MIDI in a non-realtime critical situation. DO NOT call
        this from the audio thread */
    void addMidiMessage (const MidiMessage msg);
    
    void applySettings (Settings&);
    
    void setSession (SessionPtr);
    void refreshSession();
    
    bool addGraph (RootGraph* graph);
    bool removeGraph (RootGraph* graph);
    
    void setCurrentGraph (const int index);
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
    void prepareExternalPlayback (const double sampleRate, const int blockSize,
                                  const int numIns, const int numOuts);
    void processExternalBuffers (AudioBuffer<float>& buffer, MidiBuffer& midi);
    void releaseExternalResources();
    
private:
    class Private;
    ScopedPointer<Private> priv;
    Globals& world;
};

typedef ReferenceCountedObjectPtr<AudioEngine> AudioEnginePtr;

}
