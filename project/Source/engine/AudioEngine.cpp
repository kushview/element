/*
    AudioEngine.cpp - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "engine/MidiClipSource.h"
#include "engine/Transport.h"
#include "Globals.h"

namespace Element {

RootGraph::RootGraph()
{

}
    
void RootGraph::setPlayConfigFor (AudioIODevice *device)
{
    jassert (device);
    const int numIns        = device->getActiveInputChannels().countNumberOfSetBits();
    const int numOuts       = device->getActiveOutputChannels().countNumberOfSetBits();
    const int bufferSize    = device->getCurrentBufferSizeSamples();
    const double sampleRate = device->getCurrentSampleRate();
    
    setPlayConfigDetails (numIns, numOuts, sampleRate, bufferSize);
    updateChannelNames (device);
    graphName = "Device";
}

void RootGraph::setPlayConfigFor (const DeviceManager::AudioDeviceSetup& setup)
{
    setPlayConfigDetails (setup.inputChannels.countNumberOfSetBits(),
                          setup.outputChannels.countNumberOfSetBits(),
                          setup.sampleRate, setup.bufferSize);
}

const String RootGraph::getName() const { return graphName; }
    
const String RootGraph::getInputChannelName (int c) const { return audioInputNames [c]; }
    
const String RootGraph::getOutputChannelName (int c) const { return audioOutputNames[c]; }

void RootGraph::updateChannelNames (AudioIODevice* device)
{
    auto activeIn  = device->getActiveInputChannels();
    auto namesIn   = device->getInputChannelNames();
    auto activeOut = device->getActiveOutputChannels();
    auto namesOut  = device->getOutputChannelNames();
    audioOutputNames.clear(); audioInputNames.clear();
    for (int i = 0; i < namesIn.size(); ++i)
        if (activeIn[i] == true)
            audioInputNames.add(namesIn[i]);
    for (int i = 0; i < namesOut.size(); ++i)
        if (activeOut[i] == true)
            audioOutputNames.add(namesOut[i]);
}

class AudioEngine::Private : public AudioIODeviceCallback,
                             public MidiInputCallback,
                             public ValueListener
{
public:
    Private (AudioEngine& e)
        : engine (e),sampleRate (0), blockSize (0), isPrepared (false),
          numInputChans (0), numOutputChans (0),
          tempBuffer (1, 1)
    {
        graphs.add (new RootGraph ());
        tempoValue.addListener (this);
    }

    ~Private()
    {
        tempoValue.removeListener (this);
        if (isPrepared)
        {
            jassertfalse;
            releaseResources();
            isPrepared = false;
        }
    }

    void audioDeviceIOCallback (const float** const inputChannelData, const int numInputChannels,
                                float** const outputChannelData, const int numOutputChannels,
                                const int numSamples) override
    {
        transport.preProcess (numSamples);
        jassert (sampleRate > 0 && blockSize > 0);
        incomingMidi.clear();
        
        int totalNumChans = 0;
        
        if (numInputChannels > numOutputChannels)
        {
            // if there aren't enough output channels for the number of
            // inputs, we need to create some temporary extra ones (can't
            // use the input data in case it gets written to)
            tempBuffer.setSize (numInputChannels - numOutputChannels, numSamples,
                                false, false, true);
            
            for (int i = 0; i < numOutputChannels; ++i)
            {
                channels[totalNumChans] = outputChannelData[i];
                memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * (size_t) numSamples);
                ++totalNumChans;
            }
            
            for (int i = numOutputChannels; i < numInputChannels; ++i)
            {
                channels[totalNumChans] = tempBuffer.getWritePointer (i - numOutputChannels, 0);
                memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * (size_t) numSamples);
                ++totalNumChans;
            }
        }
        else
        {
            for (int i = 0; i < numInputChannels; ++i)
            {
                channels[totalNumChans] = outputChannelData[i];
                memcpy (channels[totalNumChans], inputChannelData[i], sizeof (float) * (size_t) numSamples);
                ++totalNumChans;
            }
            
            for (int i = numInputChannels; i < numOutputChannels; ++i)
            {
                channels[totalNumChans] = outputChannelData[i];
                zeromem (channels[totalNumChans], sizeof (float) * (size_t) numSamples);
                ++totalNumChans;
            }
        }
        
        const ScopedLock sl (lock);
        auto& graph (*graphs.getUnchecked (0));
        
        const bool shouldProcess = true;
        if (shouldProcess)
        {
            const int64 remainingFrames = transport.getRemainingFrames();
            ignoreUnused (remainingFrames);
            const ScopedLock sl2 (graph.getCallbackLock());
            
            if (graph.isSuspended())
            {
                for (int i = 0; i < numOutputChannels; ++i)
                    zeromem (outputChannelData[i], sizeof (float) * (size_t) numSamples);
            }
            else
            {
                messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples);
                AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
                graph.processBlock (buffer, incomingMidi);
                if (transport.isPlaying()) {
                    transport.advance (numSamples);
                }
            }
        }
        
        transport.postProcess (numSamples);
    }
    
    void audioDeviceAboutToStart (AudioIODevice* const device) override
    {
        const double newSampleRate = device->getCurrentSampleRate();
        const int newBlockSize     = device->getCurrentBufferSizeSamples();
        const int numChansIn       = device->getActiveInputChannels().countNumberOfSetBits();
        const int numChansOut      = device->getActiveOutputChannels().countNumberOfSetBits();
        
        const ScopedLock sl (lock);
        sampleRate      = newSampleRate;
        blockSize       = newBlockSize;
        numInputChans   = numChansIn;
        numOutputChans  = numChansOut;
        
        messageCollector.reset (sampleRate);
        channels.calloc ((size_t) jmax (numChansIn, numChansOut) + 2);
        
        if (isPrepared)
        {
            isPrepared = false;
            releaseResources();
        }
        
        prepareToPlay (sampleRate, blockSize);
        isPrepared = true;
    }
    
    void audioDeviceStopped() override
    {
        const ScopedLock sl (lock);
        if (isPrepared)
            releaseResources();
        isPrepared  = false;
        sampleRate  = 0.0;
        blockSize   = 0;
        tempBuffer.setSize (1, 1);
    }
    
    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        messageCollector.addMessageToQueue (message);
    }

    void addGraph (RootGraph* graph)
    {
        if (isPrepared)
            prepareGraph (graph, sampleRate, blockSize);
        ScopedLock sl (lock);
        graphs.add (graph);
    }
    
    void removeGraph (RootGraph* graph)
    {
        {
            ScopedLock sl (lock);
            jassert (graphs.contains (graph));
            graphs.removeObject (graph, false);
        }
        
        if (isPrepared)
            graph->releaseResources();
        
        
        deleteAndZero (graph);
    }
    
    void setSession (SessionPtr s)
    {
        session = s;
        if (session)
            tempoValue.referTo (session->getPropertyAsValue (Tags::tempo));
        else
            tempoValue = tempoValue.getValue(); // drop reference?
        
        valueChanged (tempoValue);
    }
    
    void valueChanged (Value& value) override
    {
        if (tempoValue.refersToSameSourceAs (value))
        {
            const float tempo = (float) tempoValue.getValue();
            transport.requestTempo (tempo);
        }
    }
    
private:
    friend class AudioEngine;
    AudioEngine&    engine;
    Transport       transport;
    OwnedArray<RootGraph> graphs;
    SessionPtr session;
    Value tempoValue;
    Atomic<float> nextTempo;
    
    CriticalSection lock;
    double sampleRate   = 0.0;
    int blockSize       = 0;
    bool isPrepared     = false;
    int graphIndex      = -1;
    
    int numInputChans, numOutputChans;
    HeapBlock<float*> channels;
    AudioSampleBuffer tempBuffer;
    MidiBuffer incomingMidi;
    MidiMessageCollector messageCollector;
    
    void prepareGraph (RootGraph* graph, double sampleRate, int estimatedBlockSize)
    {
        graph->setPlayConfigDetails (numInputChans, numOutputChans,
                                     sampleRate, blockSize);
        graph->setPlayHead (&transport);
        graph->prepareToPlay (sampleRate, estimatedBlockSize);
    }
    
    void prepareToPlay (double sampleRate, int estimatedBlockSize)
    {
        for (auto* graph : graphs)
            prepareGraph (graph, sampleRate, estimatedBlockSize);
    }
    
    void releaseResources()
    {
        for (auto* graph : graphs)
            graph->releaseResources();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Private)
};

AudioEngine::AudioEngine (Globals& g)
    : world (g)
{
    priv = new Private (*this);
}

AudioEngine::~AudioEngine()
{
    deactivate();
    priv = nullptr;
}

void AudioEngine::activate()
{
    auto& devices (world.getDeviceManager());
    devices.addMidiInputCallback (String::empty, &getMidiInputCallback());
}

void AudioEngine::deactivate()
{
    auto& devices (world.getDeviceManager());
    devices.removeMidiInputCallback (String::empty, &getMidiInputCallback());
}

AudioIODeviceCallback&  AudioEngine::getAudioIODeviceCallback() { jassert (priv != nullptr); return *priv; }
MidiInputCallback&      AudioEngine::getMidiInputCallback()     { jassert (priv != nullptr); return *priv; }

RootGraph& AudioEngine::getRootGraph()
{
    return *getGraph(0);
}

bool AudioEngine::addGraph (RootGraph* graph)
{
    jassert (priv && graph);
    priv->addGraph (graph);
    return true;
}
    
bool AudioEngine::removeGraph (RootGraph* graph)
{
    jassert (priv && graph);
    priv->removeGraph (graph);
    return true;
}
    
RootGraph* AudioEngine::getGraph (const int index)
{
    ScopedLock sl (priv->lock);
    if (isPositiveAndBelow(index, priv->graphs.size()))
        return priv->graphs.getUnchecked (index);
    return nullptr;
}

void AudioEngine::setSession (SessionPtr session)
{
    if (priv)
        priv->setSession (session);
}
    
}
