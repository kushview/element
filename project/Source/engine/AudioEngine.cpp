/*
    AudioEngine.cpp - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#include "engine/AudioEngine.h"
#include "engine/ClipFactory.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "engine/MidiClipSource.h"
#include "engine/Transport.h"
#include "session/DeviceManager.h"
#include "Globals.h"

namespace Element {

class RootGraph : public GraphProcessor
{
public:
    RootGraph() { }
    ~RootGraph() { }
};

class  AudioEngine::Callback : public AudioIODeviceCallback,
                               public MidiInputCallback
{
public:
    Callback (AudioEngine* e)
        : engine (e),
          processor (nullptr),
          sampleRate (0),
          blockSize (0),
          isPrepared (false),
          numInputChans (0),
          numOutputChans (0),
          tempBuffer (1, 1)
    { }

    ~Callback()
    {
        setRootGraph (nullptr);
    }

    void setRootGraph (GraphProcessor* const nextGraph)
    {
        if (processor != nextGraph)
        {
            if (nextGraph != nullptr && sampleRate > 0 && blockSize > 0)
            {
                nextGraph->setPlayHead (engine->transport());
                nextGraph->setPlayConfigDetails (numInputChans, numOutputChans,
                                                 sampleRate, blockSize);
                nextGraph->prepareToPlay (sampleRate, blockSize);
            }

            GraphProcessor* oldOne;

            {
                const ScopedLock sl (lock);
                oldOne = isPrepared ? processor : nullptr;
                processor = nextGraph;

                if (processor != nullptr)
                    processor->setPlayHead (engine->transport());

                isPrepared = true;
            }

            if (oldOne != nullptr) {
                oldOne->setPlayHead (nullptr);
                oldOne->releaseResources();
            }
        }
    }

    void audioDeviceIOCallback (const float** const inputChannelData, const int numInputChannels,
                                float** const outputChannelData, const int numOutputChannels,
                                const int numSamples) override
    {
        engine->transport()->preProcess (numSamples);
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

        if (processor != nullptr)
        {
            const int remainingFrames = engine->transport()->getRemainingFrames();
            ignoreUnused (remainingFrames);
            const ScopedLock sl2 (processor->getCallbackLock());

            if (processor->isSuspended())
            {
                for (int i = 0; i < numOutputChannels; ++i)
                    zeromem (outputChannelData[i], sizeof (float) * (size_t) numSamples);
            }
            else
            {
                messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples);
                AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
                processor->processBlock (buffer, incomingMidi);
                if (engine->transport()->isPlaying()) {
                    engine->transport()->advance (numSamples);
                }
            }
        }
        
        engine->transport()->postProcess (numSamples);
    }

    void audioDeviceAboutToStart (AudioIODevice* const device) override
    {
        const double newSampleRate = device->getCurrentSampleRate();
        const int newBlockSize     = device->getCurrentBufferSizeSamples();
        const int numChansIn       = device->getActiveInputChannels().countNumberOfSetBits();
        const int numChansOut      = device->getActiveOutputChannels().countNumberOfSetBits();

        const ScopedLock sl (lock);

        sampleRate = newSampleRate;
        blockSize  = newBlockSize;
        numInputChans  = numChansIn;
        numOutputChans = numChansOut;

        messageCollector.reset (sampleRate);
        channels.calloc ((size_t) jmax (numChansIn, numChansOut) + 2);

        if (processor != nullptr)
        {
            if (isPrepared)
                processor->releaseResources();

            processor->setPlayConfigDetails (numChansIn, numChansOut, sampleRate, blockSize);
            GraphProcessor* const oldProcessor = processor;
            setRootGraph (nullptr);
            setRootGraph (oldProcessor);
        }
    }

    void audioDeviceStopped() override
    {
        const ScopedLock sl (lock);

        if (processor != nullptr && isPrepared)
            processor->releaseResources();

        sampleRate = 0.0;
        blockSize = 0;
        isPrepared = false;
        tempBuffer.setSize (1, 1);
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        messageCollector.addMessageToQueue (message);
    }

private:
    AudioEngine* engine;
    GraphProcessor* processor;
    CriticalSection lock;
    double sampleRate;
    int blockSize;
    bool isPrepared;

    int numInputChans, numOutputChans;
    HeapBlock<float*> channels;
    AudioSampleBuffer tempBuffer;

    MidiBuffer incomingMidi;
    MidiMessageCollector messageCollector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Callback)
};

class AudioEngine::Private
{
public:
    Private (AudioEngine& e)
        : engine (e)
    {
        graph.setPlayHead (&transport);
        clips = new ClipFactory (e);
        clips->registerType (createMidiClipType());
    }

    ~Private()
    {
        clips = nullptr;
    }

    AudioEngine& engine;
    RootGraph graph;
    Transport transport;
    ScopedPointer<ClipFactory> clips;
};

AudioEngine::AudioEngine (Globals& g)
    : world (g)
{
    cb   = new Callback (this);
    priv = new Private (*this);
}

AudioEngine::~AudioEngine()
{
    deactivate();
    priv = nullptr;
    delete cb;
    cb = nullptr;
}

void AudioEngine::activate()
{
    auto& devices (globals().getDeviceManager());    
    devices.addMidiInputCallback (String::empty, &getMidiInputCallback());
    cb->setRootGraph (&graph());
}

void AudioEngine::deactivate()
{
    cb->setRootGraph (nullptr);
    globals().getDeviceManager().removeMidiInputCallback (String::empty, &getMidiInputCallback());
    
    priv->graph.clear();
    priv->graph.reset();
}

AudioIODeviceCallback&  AudioEngine::getAudioIODeviceCallback() { jassert (cb != nullptr); return *cb; }
MidiInputCallback&      AudioEngine::getMidiInputCallback()     { jassert (cb != nullptr); return *cb; }

ClipFactory& AudioEngine::clips()
{
    jassert (priv->clips != nullptr);
    return *priv->clips;
}

Globals& AudioEngine::globals() { return world; }

GraphProcessor& AudioEngine::graph()
{
    return priv->graph;
}

Transport* AudioEngine::transport()
{
    return &priv->transport;
}

ValueTree AudioEngine::createGraphTree()
{
    if (! priv)
        return ValueTree();
    return priv->graph.getGraphModel().createCopy();
}

void AudioEngine::restoreFromGraphTree (const ValueTree&) { }

}
