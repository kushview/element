/*
    AudioEngine.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "engine/Engine.h"
#include "engine/GraphProcessor.h"
#include "session/DeviceManager.h"
namespace Element {

class Globals;
class ClipFactory;
class EngineControl;
class GraphProcessor;
class PatternInterface;
class Pattern;
class Transport;
    
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

    ValueTree createGraphTree();
    void restoreFromGraphTree (const ValueTree&);
    
    // Member access
    Globals& globals();
    RootGraph& getRootGraph();
    Transport* transport();

    // Engine methods
    AudioIODeviceCallback& getAudioIODeviceCallback() override;
    MidiInputCallback& getMidiInputCallback() override;
    
private:
    class Private;
    ScopedPointer<Private> priv;
    Globals& world;
};

typedef ReferenceCountedObjectPtr<AudioEngine> AudioEnginePtr;

}
