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

#include <element/audioengine.hpp>
#include "engine/internalformat.hpp"
#include "engine/midiclock.hpp"
#include "engine/midichannelmap.hpp"
#include "engine/midiengine.hpp"
#include "engine/miditranspose.hpp"
#include <element/transport.hpp>
#include "engine/rootgraph.hpp"
#include <element/context.hpp>
#include <element/settings.hpp>
#include "tempo.hpp"

#include "engine/trace.hpp"

namespace element {

struct RootGraphRender : public AsyncUpdater
{
    std::function<void()> onActiveGraphChanged;

    RootGraphRender()
    {
        graphs.ensureStorageAllocated (32);
    }

    void handleAsyncUpdate() override
    {
        if (onActiveGraphChanged)
            onActiveGraphChanged();
    }

    const int setCurrentGraph (const int index)
    {
        if (index == currentGraph)
            return currentGraph;
        currentGraph = index;
        triggerAsyncUpdate();
        return currentGraph;
    }

    const int getCurrentGraphIndex() const { return currentGraph; }

    RootGraph* getCurrentGraph() const
    {
        return isPositiveAndBelow (currentGraph, graphs.size()) ? graphs.getUnchecked (currentGraph)
                                                                : nullptr;
    }

    void prepareBuffers (const int numIns, const int numOuts, const int numSamples)
    {
        numInputChans = numIns;
        numOutputChans = numOuts;
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

    void dumpGraphs()
    {
    }

    void renderGraphs (AudioSampleBuffer& buffer, MidiBuffer& midi)
    {
        if (program.wasRequested())
        {
            const int nextGraph = findGraphForProgram (program);
            if (nextGraph != currentGraph)
                setCurrentGraph (nextGraph);
            program.reset();
        }

        auto* const current = getCurrentGraph();
        auto* const last = (lastGraph >= 0 && lastGraph < graphs.size()) ? getGraph (lastGraph) : nullptr;

        if (current == nullptr || last == nullptr)
        {
            buffer.clear();
            midi.clear();
            return;
        }

        const int numSamples = buffer.getNumSamples();
        const int numChans = buffer.getNumChannels();
        const bool graphChanged = lastGraph != currentGraph;
        const bool shouldProcess = true;
        const RootGraph::RenderMode mode = current->getRenderMode();
        const bool modeChanged = graphChanged && mode != last->getRenderMode();

        if (shouldProcess)
        {
            audioOut.setSize (numChans, numSamples, false, false, true);
            audioTemp.setSize (numChans, numSamples, false, false, true);

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

                // avoids feedback loop when IO node ins are
                // connected to IO node outs
                midiTemp.clear (0, numSamples);

                if ((last == graph && graphChanged && last->isSingle())
                    || (graphChanged && current != nullptr && current->isSingle() && graph != current))
                {
                    // send kill messages to the last graph(s) when the graph changes
                    // see http://nickfever.com/music/midi-cc-list
                    for (int i = 0; i < 16; ++i)
                    {
                        // sustain pedal off
                        midiTemp.addEvent (MidiMessage::controllerEvent (i + 1, 64, 0), 0);
                        // Sostenuto off
                        midiTemp.addEvent (MidiMessage::controllerEvent (i + 1, 66, 0), 0);
                        // Hold off
                        midiTemp.addEvent (MidiMessage::controllerEvent (i + 1, 69, 0), 0);

                        midiTemp.addEvent (MidiMessage::allNotesOff (i + 1), 0);
                    }
                }
                else if ((current == graph && graph->isSingle())
                         || (current != nullptr && ! current->isSingle() && ! graph->isSingle()))
                {
                    // current single graph or parallel graphs get MIDI always
                    midiTemp.addEvents (midi, 0, numSamples, 0);
                }

                {
                    MidiBuffer* tmpArray[] = { &midiTemp };
                    MidiPipe midiPipe (tmpArray, 1);

                    const ScopedLock sl (graph->getPropertyLock());
                    if (graph->isSuspended())
                    {
                        graph->renderBypassed (audioTemp, midiPipe);
                    }
                    else
                    {
                        graph->render (audioTemp, midiPipe);
                    }
                }

                if (graphChanged && ((current->isSingle() && current != graph) || (modeChanged && ! current->isSingle() && graph->isSingle())))

                {
                    // DBG("  FADE OUT LAST GRAPH: " << graph->engineIndex);
                    for (int i = 0; i < numOutputChans; ++i)
                        audioOut.addFromWithRamp (i, 0, audioTemp.getReadPointer (i), numSamples, 1.f, 0.f);
                }
                else if ((graph == current && graph->isSingle()) || (! graph->isSingle() && (current != nullptr) && ! current->isSingle()))
                {
                    // if it's the current single graph or both are parallel...
                    if (graphChanged && (graph->isSingle() || (modeChanged && ! graph->isSingle() && ! current->isSingle())))
                    {
                        // DBG("  FADE IN NEW GRAPH: " << graph->engineIndex);
                        for (int i = 0; i < numOutputChans; ++i)
                            audioOut.addFromWithRamp (i, 0, audioTemp.getReadPointer (i), numSamples, 0.f, 1.f);
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

            // setup a program change if present
            for (auto m : midi)
            {
                auto msg = m.getMessage();
                if (m.samplePosition >= numSamples)
                    break;
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
            midi.clear();
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                zeromem (buffer.getWritePointer (i), sizeof (float) * (size_t) numSamples);
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

    /** not realtime safe! AudioEngine's callback should be locked when you call this */
    void removeGraph (RootGraph* graph)
    {
        jassert (graphs.contains (graph));
        graphs.removeFirstMatchingValue (graph);
        graph->engineIndex = -1;
        updateIndexes();
        if (currentGraph >= graphs.size())
            currentGraph = graphs.size() - 1;
        if (lastGraph >= graphs.size())
            lastGraph = graphs.size() - 1;
    }

    int size() const { return graphs.size(); }

    RootGraph* getGraph (const int i) const { return graphs.getUnchecked (i); }
    int getGraphIndex() const { return currentGraph; }
    const Array<RootGraph*>& getGraphs() const { return graphs; }

private:
    Array<RootGraph*> graphs;
    int currentGraph = -1;
    int lastGraph = -1;

    struct ProgramRequest
    {
        int program = -1;
        int channel = -1;

        const bool wasRequested() const { return program >= 0; }
        void reset()
        {
            program = channel = -1;
        }

    } program;

    int numInputChans = -1;
    int numOutputChans = -1;
    AudioSampleBuffer audioOut, audioTemp;

    MidiBuffer midiOut, midiTemp;

    void updateIndexes()
    {
        for (int i = 0; i < graphs.size(); ++i)
            graphs.getUnchecked (i)->engineIndex = i;
    }

    int findGraphForProgram (const ProgramRequest& r) const
    {
        if (isPositiveAndBelow (program.program, 128))
        {
            for (int i = 0; i < graphs.size(); ++i)
            {
                auto* const g = graphs.getUnchecked (i);
                if (g->midiProgram == r.program && g->acceptsMidiChannel (program.channel))
                    return g->engineIndex;
            }
        }

        return currentGraph;
    }
};

class AudioEngine::Private : public AudioIODeviceCallback,
                             public MidiInputCallback,
                             public Value::Listener,
                             public MidiClock::Listener,
                             public Timer
{
public:
    Private (AudioEngine& e)
        : engine (e),
          sampleRate (0),
          blockSize (0),
          isPrepared (false),
          numInputChans (0),
          numOutputChans (0),
          tempBuffer (1, 1)
    {
        tempoValue.addListener (this);
        externalClockValue.addListener (this);
        currentGraph.set (-1);
        processMidiClock.set (0);
        sessionWantsExternalClock.set (0);
        midiClock.addListener (this);
        graphs.onActiveGraphChanged = std::bind (&AudioEngine::Private::onCurrentGraphChanged, this);
        midiIOMonitor = new MidiIOMonitor();
        startTimerHz (90);
    }

    ~Private()
    {
        graphs.onActiveGraphChanged = nullptr;
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

    void timerCallback() override
    {
        midiIOMonitor->notify();
    }

    RootGraph* getCurrentGraph() const { return graphs.getCurrentGraph(); }

    void onCurrentGraphChanged()
    {
        int currentGraph = -1;
        {
            ScopedLock sl (lock);
            currentGraph = graphs.getGraphIndex();
        }

        auto session = engine.context().session();
        if (currentGraph >= 0 && currentGraph != session->getActiveGraphIndex())
        {
            // NOTE: this is a cheap way to refresh the GUI, in the future this
            // will need to be smarter by determining whether or not EC needs to
            // handle the change at the model layer.
            auto graphs = session->data().getChildWithName (tags::graphs);
            graphs.setProperty (tags::active, currentGraph, nullptr);
        }
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        jassert (sampleRate > 0 && blockSize > 0);
        int totalNumChans = 0;
        ScopedNoDenormals denormals;

        for (int c = 0; c < numInputChannels; ++c)
            inMeters.getObjectPointerUnchecked (c)->updateLevel (inputChannelData, c, numSamples);

        if (numInputChannels > numOutputChannels)
        {
            // if there aren't enough output channels for the number of
            // inputs, we need to create some temporary extra ones (can't
            // use the input data in case it gets written to)
            tempBuffer.setSize (numInputChannels - numOutputChannels, numSamples, false, false, true);

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

        const bool wasPlaying = transport.isPlaying();
        AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
        processCurrentGraph (buffer, incomingMidi);

        {
            ScopedLock lockMidiOut (engine.world.midi().getMidiOutputLock());
            if (auto* const midiOut = engine.world.midi().getDefaultMidiOutput())
            {
                if (sendMidiClockToInput.get() != 1 && generateMidiClock.get() == 1)
                {
                    if (wasPlaying != transport.isPlaying())
                    {
                        if (transport.isPlaying())
                        {
                            incomingMidi.addEvent (transport.getPositionFrames() <= 0
                                                       ? MidiMessage::midiStart()
                                                       : MidiMessage::midiContinue(),
                                                   0);
                        }
                        else
                        {
                            incomingMidi.addEvent (MidiMessage::midiStop(), 0);
                        }
                    }

                    midiClockMaster.setTempo (transport.getTempo());
                    midiClockMaster.render (incomingMidi, numSamples);
                }

                const double delayMs = midiOutLatency.get();
                if (! incomingMidi.isEmpty())
                {
                    midiIOMonitor->sent();
#if JUCE_WINDOWS
                    midiOut->sendBlockOfMessagesNow (incomingMidi);
#else
                    midiOut->sendBlockOfMessages (incomingMidi, delayMs + Time::getMillisecondCounterHiRes(), sampleRate);
#endif
                }
            }
        }

        for (int c = 0; c < numOutputChannels; ++c)
            outMeters.getObjectPointerUnchecked (c)->updateLevel (outputChannelData, c, numSamples);
        incomingMidi.clear();
    }

    void processCurrentGraph (AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        const int numSamples = buffer.getNumSamples();
        messageCollector.removeNextBlockOfMessages (midi, numSamples);
        // element::traceMidi (midi);

        const ScopedLock sl (lock);
        const bool shouldProcess = shouldBeLocked.get() == 0;
        const bool wasPlaying = transport.isPlaying();
        transport.preProcess (numSamples);

        if (shouldProcess)
        {
            if (generateMidiClock.get() == 1 && sendMidiClockToInput.get() == 1)
            {
                if (wasPlaying != transport.isPlaying())
                {
                    if (transport.isPlaying())
                    {
                        midi.addEvent (transport.getPositionFrames() <= 0
                                           ? MidiMessage::midiStart()
                                           : MidiMessage::midiContinue(),
                                       0);
                    }
                    else
                    {
                        midi.addEvent (MidiMessage::midiStop(), 0);
                    }
                }
                midiClockMaster.setTempo (static_cast<double> (transport.getTempo()));
                midiClockMaster.render (midi, numSamples);
            }

            if (currentGraph.get() != graphs.getCurrentGraphIndex())
                graphs.setCurrentGraph (currentGraph.get());
            graphs.renderGraphs (buffer, midi); // user requested index can be cancelled by program changed
            currentGraph.set (graphs.getCurrentGraphIndex());
        }
        else
        {
            for (int i = 0; i < buffer.getNumChannels(); ++i)
                zeromem (buffer.getWritePointer (i), sizeof (float) * (size_t) numSamples);
        }

        if (transport.isPlaying())
            transport.advance (numSamples);

        transport.postProcess (numSamples);
    }

    bool isTimeMaster() const
    {
        if (engine.getRunMode() == RunMode::Plugin)
            return sessionWantsExternalClock.get() == 0;
        return processMidiClock.get() == 0 && sessionWantsExternalClock.get() == 0;
    }

    void audioDeviceAboutToStart (AudioIODevice* const device) override
    {
        const double newSampleRate = device->getCurrentSampleRate();
        const int newBlockSize = device->getCurrentBufferSizeSamples();
        const int numChansIn = device->getActiveInputChannels().countNumberOfSetBits();
        const int numChansOut = device->getActiveOutputChannels().countNumberOfSetBits();
        audioAboutToStart (newSampleRate, newBlockSize, numChansIn, numChansOut);
    }

    void audioAboutToStart (const double newSampleRate, const int newBlockSize, const int numChansIn, const int numChansOut)
    {
        const ScopedLock sl (lock);

        sampleRate = newSampleRate;
        blockSize = newBlockSize;
        numInputChans = numChansIn;
        numOutputChans = numChansOut;

        midiClock.reset (sampleRate, blockSize);
        messageCollector.reset (sampleRate);
        keyboardState.addListener (&messageCollector);
        channels.calloc ((size_t) jmax (numChansIn, numChansOut) + 2);

        graphs.prepareBuffers (numInputChans, numOutputChans, blockSize);

        while (inMeters.size() < numInputChans)
            inMeters.add (new AudioEngine::LevelMeter());
        while (outMeters.size() < numOutputChans)
            outMeters.add (new AudioEngine::LevelMeter());

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
    }

    void audioStopped()
    {
        const ScopedLock sl (lock);
        keyboardState.removeListener (&messageCollector);
        if (isPrepared)
            releaseResources();
        isPrepared = false;
        sampleRate = 0.0;
        blockSize = 0;
        tempBuffer.setSize (1, 1);
        graphs.releaseBuffers();
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        if (! message.isActiveSense() && ! message.isMidiClock())
            midiIOMonitor->received();
        messageCollector.addMessageToQueue (message);
        const bool clockWanted = processMidiClock.get() > 0 && sessionWantsExternalClock.get() > 0;
        if (clockWanted && message.isMidiClock())
        {
            midiClock.process (message);
        }
        else if (clockWanted && message.isMidiStart())
        {
            transport.requestPlayState (true);
            transport.requestAudioFrame (0);
        }
        else if (clockWanted && message.isMidiStop())
        {
            transport.requestPlayState (false);
        }
        else if (clockWanted && message.isMidiContinue())
        {
            transport.requestPlayState (true);
        }
    }

    void addGraph (RootGraph* graph)
    {
        jassert (graph);
        if (isPrepared)
            prepareGraph (graph, sampleRate, blockSize);
        ScopedLock sl (lock);
        if (graphs.addGraph (graph))
        {
            graph->renderingSequenceChanged.connect (
                std::bind (&AudioEngine::updateExternalLatencySamples, &engine));
        }
    }

    void removeGraph (RootGraph* graph)
    {
        {
            ScopedLock sl (lock);
            graphs.removeGraph (graph);
        }

        graph->renderingSequenceChanged.disconnect_all_slots();
        if (isPrepared)
            graph->releaseResources();
    }

    void connectSessionValues()
    {
        if (session)
        {
            tempoValue.referTo (session->getPropertyAsValue (tags::tempo));
            externalClockValue.referTo (session->getPropertyAsValue ("externalSync"));
            transport.requestMeter (session->getProperty (tags::beatsPerBar, 4),
                                    session->getProperty (tags::beatDivisor, 2));
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
            const bool wantsClock = (bool) value.getValue();
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

    void midiClockSignalAcquired() override {}
    void midiClockSignalDropped() override {}

    bool isUsingExternalClock() const
    {
        if (engine.getRunMode() == RunMode::Plugin)
            return sessionWantsExternalClock.get() > 0;
        return sessionWantsExternalClock.get() > 0 && processMidiClock.get() > 0;
    }

private:
    friend class AudioEngine;
    AudioEngine& engine;
    Transport transport;
    RootGraphRender graphs;
    SessionPtr session;

    Value tempoValue;
    Atomic<float> nextTempo;

    CriticalSection lock;
    double sampleRate = 0.0;
    int blockSize = 0;
    bool isPrepared = false;
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
    Atomic<int> generateMidiClock { 0 };
    Atomic<int> sendMidiClockToInput { 0 };

    MidiClock midiClock;
    MidiClockMaster midiClockMaster;

    int latencySamples = 0;

    // GraphRender::locked must match this default value
    Atomic<int> shouldBeLocked { 0 };

    MidiIOMonitorPtr midiIOMonitor;

    Atomic<double> midiOutLatency { 0.0 };

    ReferenceCountedArray<AudioEngine::LevelMeter> inMeters, outMeters;

    void prepareGraph (RootGraph* graph, double sampleRate, int estimatedBlockSize)
    {
        graph->setRenderDetails (sampleRate, blockSize);
        graph->setPlayHead (&transport);
        graph->prepareToRender (sampleRate, estimatedBlockSize);
    }

    void prepareToPlay (double sampleRate, int estimatedBlockSize)
    {
        midiClockMaster.setSampleRate (sampleRate);
        midiClockMaster.setTempo (transport.getTempo());
        for (int i = 0; i < graphs.size(); ++i)
            prepareGraph (graphs.getGraph (i), sampleRate, estimatedBlockSize);
    }

    void releaseResources()
    {
        for (int i = 0; i < graphs.size(); ++i)
            graphs.getGraph (i)->releaseResources();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Private)
};

AudioEngine::AudioEngine (Context& g, RunMode m)
    : world (g), runMode (m)
{
    priv = std::make_unique<Private> (*this);
}

AudioEngine::~AudioEngine() noexcept
{
    deactivate();
    priv.reset();
}

void AudioEngine::activate()
{
    if (getRunMode() == RunMode::Standalone)
    {
        auto& midi (world.midi());
        midi.addMidiInputCallback (&getMidiInputCallback());
    }
}

void AudioEngine::deactivate()
{
    if (getRunMode() == RunMode::Standalone)
    {
        auto& midi (world.midi());
        midi.removeMidiInputCallback (&getMidiInputCallback());
    }
}

AudioIODeviceCallback& AudioEngine::getAudioIODeviceCallback()
{
    jassert (priv != nullptr);
    return *priv;
}
MidiInputCallback& AudioEngine::getMidiInputCallback()
{
    jassert (priv != nullptr);
    return *priv;
}

bool AudioEngine::addGraph (RootGraph* graph)
{
    jassert (priv && graph);
    priv->addGraph (graph);
    return true;
}

void AudioEngine::applySettings (Settings& settings)
{
    const bool useMidiClock = settings.getUserSettings()->getValue ("clockSource") == "midiClock";
    if (useMidiClock)
        priv->resetMidiClock();
    priv->processMidiClock.set (useMidiClock ? 1 : 0);
    priv->generateMidiClock.set (settings.generateMidiClock() ? 1 : 0);
    priv->sendMidiClockToInput.set (settings.sendMidiClockToInput() ? 1 : 0);
    priv->midiOutLatency.set (settings.getMidiOutLatency());
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

void AudioEngine::addMidiMessage (const MidiMessage msg, bool handleOnDeviceQueue)
{
    if (priv == nullptr)
        return;
    if (handleOnDeviceQueue)
        priv->handleIncomingMidiMessage (nullptr, msg);
    else
        priv->messageCollector.addMessageToQueue (msg);
}

void AudioEngine::setActiveGraph (const int index)
{
    if (priv == nullptr)
        return;
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
    jassert (priv);
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

void AudioEngine::prepareExternalPlayback (const double sampleRate, const int blockSize, const int numIns, const int numOuts)
{
    if (priv)
        priv->audioAboutToStart (sampleRate, blockSize, numIns, numOuts);
}

void AudioEngine::processExternalBuffers (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    if (priv)
    {
        if (getRunMode() == RunMode::Plugin)
            world.midi().processMidiBuffer (midi, buffer.getNumSamples(), priv->sampleRate);
        priv->processCurrentGraph (buffer, midi);
    }
}

bool AudioEngine::isUsingExternalClock() const
{
    return priv && priv->isUsingExternalClock();
}

void AudioEngine::processExternalPlayhead (AudioPlayHead* playhead, const int nframes)
{
    auto& transport (priv->transport);
    if (auto pos = playhead->getPosition())
    {
        if (auto bpm = pos->getBpm())
            transport.requestTempo (*pos->getBpm());
        if (auto timesig = pos->getTimeSignature())
        {
            transport.requestMeter (timesig->numerator, BeatType::fromDivisor (timesig->denominator));
        }
        transport.requestPlayState (pos->getIsPlaying());
        transport.requestRecordState (pos->getIsRecording());
        if (auto frameTime = pos->getTimeInSamples())
            if (transport.getPositionFrames() != *frameTime)
                transport.requestAudioFrame (*frameTime);
    }
    transport.preProcess (0);
    transport.postProcess (0);
}

void AudioEngine::releaseExternalResources()
{
    if (priv)
        priv->audioStopped();
}

Context& AudioEngine::context() const { return world; }

void AudioEngine::updateExternalLatencySamples()
{
    int latencySamples = 0;

    {
        ScopedLock sl (priv->lock);

        auto* current = priv->getCurrentGraph();
        if (nullptr == current)
            return;

        if (current->getRenderMode() == RootGraph::SingleGraph)
        {
            latencySamples = current->getLatencySamples();
        }
        else
        {
            for (auto* const graph : priv->graphs.getGraphs())
                if (graph->getRenderMode() == RootGraph::Parallel)
                    latencySamples = jmax (latencySamples, graph->getLatencySamples());
        }
    }

    priv->latencySamples = latencySamples;
    sampleLatencyChanged();
}

int AudioEngine::getExternalLatencySamples() const
{
    return priv != nullptr ? priv->latencySamples : 0;
}

MidiIOMonitorPtr AudioEngine::getMidiIOMonitor() const
{
    return priv != nullptr ? priv->midiIOMonitor : nullptr;
}

int AudioEngine::getNumChannels (bool input) const noexcept
{
    return input ? priv->numInputChans : priv->numOutputChans;
}

AudioEngine::LevelMeterPtr AudioEngine::getLevelMeter (int channel, bool input)
{
    auto& larr = input ? priv->inMeters : priv->outMeters;
    return larr[channel];
}

void AudioEngine::LevelMeter::updateLevel (const float* const* channelData, int numChannels, int numSamples) noexcept
{
    if (getReferenceCount() <= 1)
        return;

    auto localLevel = _level.get();

    if (numChannels >= 0)
    {
        for (int j = 0; j < numSamples; ++j)
        {
            float s = 0;

            s += std::abs (channelData[numChannels][j]);

            // s /= (float) numChannels;

            const float decayFactor = 0.99992f;

            if (s > localLevel)
                localLevel = s;
            else if (localLevel > 0.001f)
                localLevel *= decayFactor;
            else
                localLevel = 0;
        }
    }
    else
    {
        localLevel = 0;
    }

    _level.set (localLevel);
}

} // namespace element
