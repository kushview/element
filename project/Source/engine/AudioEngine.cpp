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

void RootGraph::setPlayConfigFor (DeviceManager& devices)
{
    if (auto* device = devices.getCurrentAudioDevice())
        setPlayConfigFor (device);
    DeviceManager::AudioSettings setup;
    devices.getAudioDeviceSetup (setup);
    audioInName     = setup.inputDeviceName;
    audioOutName    = setup.outputDeviceName;
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
    graphName = device->getName();
    if (graphName.isEmpty()) graphName = "Device";
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

struct RootGraphRender
{
    RootGraphRender()
    {
        graphs.ensureStorageAllocated (32);
    }

    const int setCurrentGraph (const int index)
    {
        if (index == currentGraph)
            return currentGraph;
        currentGraph = index;
        return currentGraph;
    }

    const int getCurrentGraphIndex() const { return currentGraph; }

    RootGraph* getCurrentGraph() const
    { 
        return isPositiveAndBelow (currentGraph, graphs.size()) ? graphs.getUnchecked(currentGraph) 
                                                                : nullptr; 
    }

    void prepareBuffers (const int numIns, const int numOuts, const int numSamples)
    {
        numInputChans   = numIns;
        numOutputChans  = numOuts;
        audioTemp.setSize (jmax (numIns, numOuts), numSamples);
        audioOut.setSize (audioTemp.getNumChannels(), audioTemp.getNumSamples());
    }

    void releaseBuffers()
    {
        numInputChans = numOutputChans = 0;
        midiOut.clear();
        midiTemp.clear();
        audioTemp.setSize (1, 1);
        audioOut.setSize (1, 1);
    }

    void renderGraphs (AudioSampleBuffer& buffer, MidiBuffer& midi)
    {
        if (program.wasRequested())
        {
            DBG("[EL] program was requested: " << program.program << " channel: " << program.channel);
            const int nextGraph = findGraphForProgram (program);
            DBG("[EL] current graph: " << currentGraph);
            DBG("[EL] selected graph: " << nextGraph);
            setCurrentGraph (nextGraph);
            program.reset();
        }
        
        const int numSamples = buffer.getNumSamples();
        const int numChans   = buffer.getNumChannels();
        auto* const current  = getCurrentGraph();
        auto* const last     = (lastGraph >= 0 && lastGraph < graphs.size()) ? getGraph(lastGraph) : nullptr;
        
        if (current == nullptr || last == nullptr)
        {
            buffer.clear();
            midi.clear();
            return;
        }

        const bool graphChanged = lastGraph != currentGraph;
        const bool shouldProcess = true;
        const RootGraph::RenderMode mode = current->getRenderMode();
        const bool modeChanged = graphChanged && mode != last->getRenderMode();

        if (shouldProcess)
        {
            // clear the mixing area
            for (int i = numChans; --i >= 0;)
                audioOut.clear (i, 0, numSamples);
            midiOut.clear();
            
            for (auto* const graph : graphs)
            {
                // copy inputs, clear outs if more than input count
                for (int i = 0; i < numInputChans; ++i)
                    audioTemp.copyFrom (i, 0, buffer, i, 0, numSamples);
                for (int i = numInputChans; i < numChans; ++i)
                    audioTemp.clear (i, 0, numSamples);
                
                if ((last == graph && graphChanged && last->isSingle())
                    || (graphChanged && current != nullptr && current->isSingle() && graph != current))
                {
                    // send all notes off to the last graph if it is single
                    // it to parellel graphs as well
                    for (int i = 0; i < 16; ++i)
                        midiTemp.addEvent (MidiMessage::allNotesOff (i + 1), 0);
                }
                else if ((current == graph && graph->isSingle()) 
                            || (current != nullptr && !current->isSingle() && !graph->isSingle()))
                {
                    // current single graph or parallel graphs get MIDI always
                    midiTemp.addEvents (midi, 0, numSamples, 0);
                }

                {
                    const ScopedLock sl (graph->getCallbackLock());
                    if (graph->isSuspended())
                    {
                        graph->processBlockBypassed (audioTemp, midiTemp);
                    }
                    else
                    {
                        graph->processBlock (audioTemp, midiTemp);
                    }
                }
                
                if (graphChanged && ((current->isSingle() && current != graph) ||
                                     (modeChanged && !current->isSingle() && graph->isSingle())))
                                     
                {
                    // DBG("  FADE OUT LAST GRAPH: " << graph->engineIndex);
                    for (int i = 0; i < numOutputChans; ++i)
                            audioOut.addFromWithRamp (i, 0, audioTemp.getReadPointer (i), 
                                                      numSamples, 1.f, 0.f);
                }
                else if ((graph == current && graph->isSingle()) ||
                         (!graph->isSingle() && (current != nullptr) && !current->isSingle()))
                {
                    // if it's the current single graph or both are parallel...
                    
                    if (graphChanged && (graph->isSingle() || 
                                        (modeChanged && !graph->isSingle() && !current->isSingle())))
                    {
                        // DBG("  FADE IN NEW GRAPH: " << graph->engineIndex);
                        for (int i = 0; i < numOutputChans; ++i)
                            audioOut.addFromWithRamp (i, 0, audioTemp.getReadPointer (i), 
                                                      numSamples, 0.f, 1.f);
                    }
                    else
                    {
                        for (int i = 0; i < numOutputChans; ++i)
                            audioOut.addFrom (i, 0, audioTemp, i, 0, numSamples);
                    }
                    
                    midiOut.addEvents (midiTemp, 0, numSamples, 0);
                }
            }

            for (int i = 0; i < numChans; ++i)
                buffer.copyFrom (i, 0, audioOut, i, 0, numSamples);

            MidiBuffer::Iterator iter (midi);
            MidiMessage msg; int frame = 0;
            
            // setup a program change if present
            while (iter.getNextEvent (msg, frame) || frame >= numSamples)
            {
                if (! msg.isProgramChange())
                    continue;
                program.program = msg.getProgramChangeNumber();
                program.channel = msg.getChannel();
            }

            // done with input, swap it with the rendered output
            midi.swapWith (midiOut);
        }
        else
        {
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                zeromem (buffer.getWritePointer(i), sizeof (float) * (size_t) numSamples);
        }

        lastGraph = currentGraph;
    }
    
    /** not realtime safe! */
    bool addGraph (RootGraph* graph)
    {
        graphs.add (graph);
        graph->engineIndex = graphs.size() - 1;
        
        if (graph->engineIndex == 0)
        {
            setCurrentGraph (0);
            lastGraph = 0;
        }
        
        return true;
    }

    /** not realtime safe! */
    void removeGraph (RootGraph* graph)
    {
        jassert (graphs.contains (graph));
        graphs.removeFirstMatchingValue (graph);
        graph->engineIndex = -1;
        updateIndexes();
        if (currentGraph >= graphs.size())
            currentGraph = graphs.size() - 1;
    }

    int size() const { return graphs.size(); }
    RootGraph* getGraph (const int i) const { return graphs.getUnchecked(i); }
    const Array<RootGraph*>& getGraphs() const { return graphs; }
    
private:
    Array<RootGraph*> graphs;
    int currentGraph        = -1;
    int lastGraph           = -1;

    struct ProgramRequest {
        int program      = -1;
        int channel      = -1;

        const bool wasRequested() const { return program >= 0; }
        void reset()
        {
            program = channel = -1;
        }

    } program;

    int numInputChans       = -1;
    int numOutputChans      = -1;
    AudioSampleBuffer   audioOut, audioTemp;

    MidiBuffer midiOut, midiTemp;

    void updateIndexes()
    {
        for (int i = 0 ; i < graphs.size(); ++i)
            graphs.getUnchecked(i)->engineIndex = i;
    }

    int findGraphForProgram (const ProgramRequest& r) const
    {
        if (isPositiveAndBelow (program.program, 127))
            for (const auto* g : graphs)
                if (g->midiChannel == 0 || g->midiChannel == r.channel)
                    if (g->midiProgram == r.program)
                        return g->engineIndex;
                
        return currentGraph;
    }
};

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
    
    RootGraph* getCurrentGraph() const { return graphs.getCurrentGraph(); }
    
    void traceMidi (MidiBuffer& buf)
    {
        MidiBuffer::Iterator iter (buf);
        MidiMessage msg; int frame = 0;
        
        while (iter.getNextEvent (msg, frame))
        {
            if (msg.isMidiClock())
            {
                DBG("clock:");
            }
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

        AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
        processCurrentGraph (buffer, incomingMidi);
        
        if (auto* const midiOut = engine.world.getDeviceManager().getDefaultMidiOutput())
        {
            const double delayMs = 6.0;
            if (! incomingMidi.isEmpty())
                midiOut->sendBlockOfMessages (incomingMidi, delayMs + Time::getMillisecondCounterHiRes(), sampleRate);
        }
        
        incomingMidi.clear();
    }
    
    void processCurrentGraph (AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        const int numSamples = buffer.getNumSamples();
        messageCollector.removeNextBlockOfMessages (midi, numSamples);
        
        const ScopedLock sl (lock);
        const bool shouldProcess = true;
        transport.preProcess (numSamples);

        if (shouldProcess)
        {
            if (currentGraph.get() != graphs.getCurrentGraphIndex())
                graphs.setCurrentGraph (currentGraph.get());
            graphs.renderGraphs (buffer, midi);  // user requested index can be cancelled by program changed
            currentGraph.set (graphs.getCurrentGraphIndex());
        }
        else
        {
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                zeromem (buffer.getWritePointer(i), sizeof (float) * (size_t) numSamples);
        }

        if (isTimeMaster() && transport.isPlaying())
            transport.advance (numSamples);
        
        if (transport.isPlaying()) {
            
        }
        
        transport.postProcess (numSamples);
    }
    
    bool isTimeMaster() const {
        return true;
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
        
        graphs.prepareBuffers (numInputChans, numOutputChans, blockSize);

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
        graphs.releaseBuffers();
    }
    
    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        messageCollector.addMessageToQueue (message);
        if (message.isMidiClock() && processMidiClock.get() > 0 && sessionWantsExternalClock.get() > 0)
            midiClock.process (message);
    }
    
    void addGraph (RootGraph* graph)
    {
        jassert (graph);
        if (isPrepared)
            prepareGraph (graph, sampleRate, blockSize);
        ScopedLock sl (lock);
        graphs.addGraph (graph);
    }
    
    void removeGraph (RootGraph* graph)
    {
        {
            ScopedLock sl (lock);
            graphs.removeGraph (graph);
        }
        
        if (isPrepared)
            graph->releaseResources();
    }
    
    void connectSessionValues()
    {
        if (session)
        {
            tempoValue.referTo (session->getPropertyAsValue (Tags::tempo));
            externalClockValue.referTo (session->getPropertyAsValue ("externalSync"));
            
            transport.requestMeter (session->getProperty (Tags::beatsPerBar, 4),
                                    session->getProperty (Tags::beatDivisor, 2));
        }
        else
        {
            tempoValue = tempoValue.getValue();
            externalClockValue = externalClockValue.getValue();
        }
    }
    
    void setSession (SessionPtr s)
    {
        session = s;
        connectSessionValues();
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
    
    void resetMidiClock()
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
    
    bool isUsingExternalClock() const
    {
       #if EL_RUNNING_AS_PLUGIN
        return sessionWantsExternalClock.get() > 0;
       #else
        return sessionWantsExternalClock.get() > 0 && processMidiClock.get() > 0;
       #endif
    }
    
private:
    friend class AudioEngine;
    AudioEngine&        engine;
    Transport           transport;
    RootGraphRender     graphs;
    SessionPtr          session;
    
    Value tempoValue;
    Atomic<float> nextTempo;
    
    CriticalSection     lock;
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

    AudioSampleBuffer graphBuffer;
    AudioSampleBuffer graphMixBuffer;

    Value externalClockValue;
    Atomic<int> sessionWantsExternalClock;
    Atomic<int> processMidiClock;
    MidiClock midiClock;
    
    AudioPlayHead::CurrentPositionInfo hostPos, lastHostPos;
    
    void prepareGraph (RootGraph* graph, double sampleRate, int estimatedBlockSize)
    {
        graph->setPlayConfigDetails (numInputChans, numOutputChans,
                                     sampleRate, blockSize);
        graph->setPlayHead (&transport);
        graph->prepareToPlay (sampleRate, estimatedBlockSize);
    }
    
    void prepareToPlay (double sampleRate, int estimatedBlockSize)
    {
        for (int i = 0; i < graphs.size(); ++i)
            prepareGraph (graphs.getGraph(i), sampleRate, estimatedBlockSize);
    }
    
    void releaseResources()
    {
        for (int i = 0; i < graphs.size(); ++i)
            graphs.getGraph(i)->releaseResources();
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
        return priv->graphs.getGraph (index);
    return nullptr;
}

void AudioEngine::addMidiMessage (const MidiMessage msg)
{
    if (priv)
        priv->messageCollector.addMessageToQueue (msg);
}
    
void AudioEngine::setActiveGraph (const int index)
{
    if (priv)
        priv->currentGraph.set (index);
}

int AudioEngine::getActiveGraph() const { return (priv != nullptr) ? priv->currentGraph.get() : -1; }

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

void AudioEngine::setMeter (int beatsPerBar, int beatDivisor)
{
    auto& transport (priv->transport);
    transport.requestMeter (beatsPerBar, beatDivisor);
}

void AudioEngine::togglePlayPause()
{
    auto& transport (priv->transport);
    transport.requestPlayPause();
}

void AudioEngine::setPlaying (const bool shouldBePlaying)
{
    auto& transport (priv->transport);
    transport.requestPlayState (shouldBePlaying);
}

void AudioEngine::setRecording (const bool shouldBeRecording)
{
    auto& transport (priv->transport);
    transport.requestRecordState (shouldBeRecording);
}

void AudioEngine::seekToAudioFrame (const int64 frame)
{
    auto& transport (priv->transport);
    transport.requestAudioFrame (frame);
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

bool AudioEngine::isUsingExternalClock() const {
    return priv && priv->isUsingExternalClock();
}

void AudioEngine::processExternalPlayhead (AudioPlayHead* playhead, const int nframes)
{
    auto& pos (priv->hostPos);
    playhead->getCurrentPosition (pos);

    auto& transport (priv->transport);
    transport.requestTempo (pos.bpm);
    transport.requestMeter (pos.timeSigNumerator, BeatType::fromPosition (pos));
    transport.requestPlayState (pos.isPlaying);
    transport.requestRecordState (pos.isRecording);
    if (transport.getPositionFrames() != pos.timeInSamples)
        transport.requestAudioFrame (pos.timeInSamples);
    
    transport.preProcess (0);
    transport.postProcess (0);
}

void AudioEngine::releaseExternalResources()
{
    if (priv)
        priv->audioStopped();
}
    
}
