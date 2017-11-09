/*
    AudioEngine.cpp - This file is part of Element
    Copyright (C) 2016-2017 Kushview, LLC.  All rights reserved.
*/

#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "engine/MidiClipSource.h"
#include "engine/MidiClock.h"
#include "engine/Transport.h"
#include "Globals.h"
#include "Settings.h"

namespace Element {

RootGraph::RootGraph() { }

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
    
const String RootGraph::getInputChannelName (int c) const { return audioInputNames[c]; }
    
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
                             public ValueListener,
                             public MidiClock::Listener
{
public:
    Private (AudioEngine& e)
        : engine (e),sampleRate (0), blockSize (0), isPrepared (false),
          numInputChans (0), numOutputChans (0),
          tempBuffer (1, 1)
    {
        tempoValue.addListener (this);
        externalClockValue.addListener (this);
        currentGraph.set (-1);
        processMidiClock.set (0);
        sessionWantsExternalClock.set (0);
        midiClock.addListener (this);
    }

    ~Private()
    {
        midiClock.removeListener (this);
        tempoValue.removeListener (this);
        externalClockValue.removeListener (this);
        
        if (isPrepared)
        {
            jassertfalse;
            releaseResources();
            isPrepared = false;
        }
    }
    
    RootGraph* getCurrentGraph() const
    {
        return isPositiveAndBelow (currentGraph.get(), graphs.size()) ? graphs.getUnchecked(currentGraph.get())
                                                                      : nullptr;
    }
    
    void traceMidi (MidiBuffer& buf)
    {
        MidiBuffer::Iterator iter (buf);
        MidiMessage msg; int frame = 0;
        
        while (iter.getNextEvent (msg, frame))
        {
            if (msg.isNoteOn())
            {
                DBG("NOTE ON");
                
            }
            if (msg.isNoteOff())
            {
                DBG("NOTE OFF");
            }
            
            if (msg.isAllNotesOff() || msg.isAllSoundOff()) {
                DBG("got it: " << frame);
            }
        }
    }
    
    void audioDeviceIOCallback (const float** const inputChannelData, const int numInputChannels,
                                float** const outputChannelData, const int numOutputChannels,
                                const int numSamples) override
    {
        jassert (sampleRate > 0 && blockSize > 0);
        
        int totalNumChans = 0;
        ScopedNoDenormals denormals;
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
#if 0
        const ScopedLock sl (lock);
        auto* const graph = getCurrentGraph();
        const bool shouldProcess = graph != nullptr;
        
        if (shouldProcess)
        {
            const int64 remainingFrames = transport.getRemainingFrames();
            ignoreUnused (remainingFrames);
            const ScopedLock sl2 (graph->getCallbackLock());
            
            if (graph->isSuspended())
            {
                for (int i = 0; i < numOutputChannels; ++i)
                    zeromem (outputChannelData[i], sizeof (float) * (size_t) numSamples);
            }
            else
            {
                AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
                graph->processBlock (buffer, incomingMidi);
            }
        }
        else
        {
            for (int i = 0; i < numOutputChannels; ++i)
                zeromem (outputChannelData[i], sizeof (float) * (size_t)numSamples);
        }
        
        if (transport.isPlaying())
            transport.advance (numSamples);
        transport.postProcess (numSamples);
        incomingMidi.clear();
#else
        AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
        processCurrentGraph (buffer, incomingMidi);
#endif
        
    }
    
    void processCurrentGraph (AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        const int numSamples = buffer.getNumSamples();
        messageCollector.removeNextBlockOfMessages (midi, numSamples);
        
        const ScopedLock sl (lock);
        auto* const graph = getCurrentGraph();
        const bool shouldProcess = graph != nullptr;
        
        transport.preProcess (numSamples);
        
        if (shouldProcess)
        {
            const ScopedLock sl2 (graph->getCallbackLock());
            
            if (graph->isSuspended())
            {
                graph->processBlockBypassed (buffer, midi);
            }
            else
            {
                graph->processBlock (buffer, midi);
            }
        }
        else
        {
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                zeromem (buffer.getWritePointer(i), sizeof (float) * (size_t)numSamples);
        }
        
        if (transport.isPlaying())
            transport.advance (numSamples);
        transport.postProcess (numSamples);
        
        traceMidi (midi);
        
//        if (auto* e = engine.world.getDeviceManager().getDefaultMidiOutput())
//            e->sendBlockOfMessages (midi, 14.f + Time::getMillisecondCounterHiRes(), sampleRate);
//
        
        midi.clear();
    }
    
    void audioDeviceAboutToStart (AudioIODevice* const device) override
    {
        const double newSampleRate = device->getCurrentSampleRate();
        const int newBlockSize     = device->getCurrentBufferSizeSamples();
        const int numChansIn       = device->getActiveInputChannels().countNumberOfSetBits();
        const int numChansOut      = device->getActiveOutputChannels().countNumberOfSetBits();
        audioAboutToStart (newSampleRate, newBlockSize, numChansIn, numChansOut);
        
        if (auto* midi = engine.world.getDeviceManager().getDefaultMidiOutput()) {
            midi->startBackgroundThread();
        }
    }
    
    void audioAboutToStart (const double newSampleRate, const int newBlockSize,
                            const int numChansIn, const int numChansOut)
    {
        const ScopedLock sl (lock);
        
        sampleRate      = newSampleRate;
        blockSize       = newBlockSize;
        numInputChans   = numChansIn;
        numOutputChans  = numChansOut;
        
        midiClock.reset (sampleRate, blockSize);
        
        messageCollector.reset (sampleRate);
        keyboardState.addListener (&messageCollector);
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
        audioStopped();
        if (auto* midi = engine.world.getDeviceManager().getDefaultMidiOutput())
            midi->stopBackgroundThread();
    }
    
    void audioStopped()
    {
        const ScopedLock sl (lock);
        keyboardState.removeListener (&messageCollector);
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
        if (message.isMidiClock() && processMidiClock.get() > 0 && sessionWantsExternalClock.get() > 0)
            midiClock.process (message);
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
    
    void connectSessionValues()
    {
        if (session)
        {
            tempoValue.referTo (session->getPropertyAsValue (Tags::tempo));
            externalClockValue.referTo (session->getPropertyAsValue ("externalSync"));
        }
        else
        {
            tempoValue = tempoValue.getValue();
            externalClockValue = externalClockValue.getValue();
        }
    }
    
    void setSession (SessionPtr s)
    {
        auto oldSession = session;
        session = s;
        connectSessionValues();
        
        // If it's the same object, then don't replace graphs
        // FIXME: need better management of root graphs
        if (oldSession == session)
            return;
        
        OwnedArray<RootGraph> newGraphs;
        
        if (session)
        {
            const int numGraphs = session->getNumGraphs();
            while (newGraphs.size() < numGraphs)
            {
                auto* graph = newGraphs.add (new RootGraph());
                if (isPrepared)
                    prepareGraph (graph, sampleRate, blockSize);
            }
        }
        else
        {
            //noop
        }
        
        ScopedLock sl (lock);
        graphs.swapWith (newGraphs);
    }
    
    void valueChanged (Value& value) override
    {
        if (tempoValue.refersToSameSourceAs (value))
        {
            const float tempo = (float) tempoValue.getValue();
            if (sessionWantsExternalClock.get() <= 0 || processMidiClock.get() <= 0)
                transport.requestTempo (tempo);
        }
        else if (externalClockValue.refersToSameSourceAs (value))
        {
            const bool wantsClock = (bool)value.getValue();
            if (wantsClock)
            {
                resetMidiClock();
            }
            else
            {
                transport.requestTempo ((float) tempoValue.getValue());
            }
            
            sessionWantsExternalClock.set (wantsClock ? 1 : 0);
        }
    }
    
    void resetMidiClock ()
    {
        midiClock.reset (sampleRate, blockSize);
    }
    
    void midiClockTempoChanged (const float bpm) override
    {
        if (sessionWantsExternalClock.get() > 0 && processMidiClock.get() > 0)
            transport.requestTempo (bpm);
    }
    
    void midiClockSignalAcquired()  override { }
    void midiClockSignalDropped()   override { }
    
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
    Atomic<int> currentGraph;
    int numInputChans, numOutputChans;
    HeapBlock<float*> channels;
    AudioSampleBuffer tempBuffer;
    MidiBuffer incomingMidi;
    MidiMessageCollector messageCollector;
    MidiKeyboardState keyboardState;
    
    Value externalClockValue;
    Atomic<int> sessionWantsExternalClock;
    Atomic<int> processMidiClock;
    MidiClock midiClock;
    
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
   #if ! EL_RUNNING_AS_PLUGIN
    auto& devices (world.getDeviceManager());
    devices.addMidiInputCallback (String::empty, &getMidiInputCallback());
   #endif
}

void AudioEngine::deactivate()
{
   #if ! EL_RUNNING_AS_PLUGIN
    auto& devices (world.getDeviceManager());
    devices.removeMidiInputCallback (String::empty, &getMidiInputCallback());
   #endif
}

AudioIODeviceCallback&  AudioEngine::getAudioIODeviceCallback() { jassert (priv != nullptr); return *priv; }
MidiInputCallback&      AudioEngine::getMidiInputCallback()     { jassert (priv != nullptr); return *priv; }

bool AudioEngine::addGraph (RootGraph* graph)
{
    jassert (priv && graph);
    priv->addGraph (graph);
    return true;
}

void AudioEngine::applySettings (Settings& settings)
{
    const bool useMidiClock = settings.getUserSettings()->getValue("clockSource") == "midiClock";
    if (useMidiClock)
        priv->resetMidiClock();
    priv->processMidiClock.set (useMidiClock ? 1 : 0);
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
    if (isPositiveAndBelow (index, priv->graphs.size()))
        return priv->graphs.getUnchecked (index);
    return nullptr;
}

void AudioEngine::addMidiMessage (const MidiMessage msg)
{
    if (priv)
        priv->messageCollector.addMessageToQueue (msg);
}
    
void AudioEngine::setCurrentGraph (const int index)
{
    if (priv)
        priv->currentGraph.set (index);
}

void AudioEngine::setSession (SessionPtr session)
{
    if (priv)
        priv->setSession (session);
}

void AudioEngine::refreshSession()
{
    if (priv)
        priv->connectSessionValues();
}

MidiKeyboardState& AudioEngine::getKeyboardState()
{
    jassert(priv);
    return priv->keyboardState;
}
    
Transport::MonitorPtr AudioEngine::getTransportMonitor() const
{
    return (priv != nullptr) ? priv->transport.getMonitor() : nullptr;
}
    
    
    
void AudioEngine::prepareExternalPlayback (const double sampleRate, const int blockSize,
                                           const int numIns, const int numOuts)
{
    if (priv)
        priv->audioAboutToStart (sampleRate, blockSize, numIns, numOuts);
}

void AudioEngine::processExternalBuffers (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    if (priv)
        priv->processCurrentGraph (buffer, midi);
}

void AudioEngine::releaseExternalResources()
{
    if (priv)
        priv->audioStopped();
}
    
}
