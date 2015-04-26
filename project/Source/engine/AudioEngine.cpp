/*
    AudioEngine.cpp - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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



#include "../EngineControl.h"
#include "../Globals.h"
#include "../MediaManager.h"

#include "AudioEngine.h"
#include "InternalFormat.h"
#include "Transport.h"

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
        AudioEngine* engine;

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

        //==============================================================================
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

                if (oldOne != nullptr)
                    oldOne->releaseResources();
            }
        }

        //==============================================================================
        void audioDeviceIOCallback (const float** const inputChannelData, const int numInputChannels,
                                    float** const outputChannelData, const int numOutputChannels,
                                    const int numSamples)
        {
            engine->transport()->preProcess (numSamples);

            jassert (sampleRate > 0 && blockSize > 0);

            incomingMidi.clear();
            messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples);
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

            AudioSampleBuffer buffer (channels, totalNumChans, numSamples);

            const ScopedLock sl (lock);

            if (processor != nullptr)
            {
                const ScopedLock sl2 (processor->getCallbackLock());

                if (processor->isSuspended())
                {
                    for (int i = 0; i < numOutputChannels; ++i)
                        zeromem (outputChannelData[i], sizeof (float) * (size_t) numSamples);
                }
                else if (engine->transport()->getRemainingFrames() >= numSamples)
                {
                    processor->processBlock (buffer, incomingMidi);
                }
            }

            if (engine->transport()->isPlaying()) {
                engine->transport()->advance (numSamples);
            }

            engine->transport()->postProcess (numSamples);
        }

        void audioDeviceAboutToStart (AudioIODevice* const device)
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

                GraphProcessor* const oldProcessor = processor;
                setRootGraph (nullptr);
                setRootGraph (oldProcessor);
            }
        }

        void audioDeviceStopped()
        {
            const ScopedLock sl (lock);

            if (processor != nullptr && isPrepared)
                processor->releaseResources();

            sampleRate = 0.0;
            blockSize = 0;
            isPrepared = false;
            tempBuffer.setSize (1, 1);
        }

        void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message)
        {
            messageCollector.addMessageToQueue (message);
        }

    private:

        //==============================================================================
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
        }

        ~Private()
        {
            controller.reset();
            clips = nullptr;
        }

        AudioEngine& engine;
        Shared<EngineControl> controller;
        RootGraph graph;
        Transport transport;

        ScopedPointer<ClipFactory> clips;
    };


    AudioEngine::AudioEngine (Globals& g)
        : world (g)
    {
        cb = new Callback (this);
        priv = new Private (*this);

    }

    AudioEngine::~AudioEngine()
    {
        deactivate();
        priv = nullptr;
        delete cb;
        cb = nullptr;
    }



    void
    AudioEngine::activate()
    {
        cb->setRootGraph (&graph());
        globals().devices().addMidiInputCallback (String::empty, dynamic_cast<MidiInputCallback*> (&this->callback()));
        Shared<EngineControl> c (controller());

        InternalFormat* fmt = globals().plugins().format<InternalFormat>();
        c->addFilter (fmt->description (InternalFormat::audioInputDevice));
        c->addFilter (fmt->description (InternalFormat::audioOutputDevice));
        c->addFilter (fmt->description (InternalFormat::midiInputDevice));
    }

    AudioIODeviceCallback&
    AudioEngine::callback()
    {
        assert (cb != nullptr);
        return *cb;
    }

    ClipFactory&
    AudioEngine::clips()
    {
        assert (priv->clips != nullptr);
        return *priv->clips;
    }

    Shared<EngineControl>
    AudioEngine::controller()
    {
        if (priv->controller == nullptr)
            priv->controller = Shared<EngineControl> (new EngineControl (*this));
        return priv->controller;
    }

    void
    AudioEngine::deactivate()
    {
        cb->setRootGraph (nullptr);
        globals().devices().removeMidiInputCallback (String::empty, dynamic_cast<MidiInputCallback*> (&this->callback()));
        priv->controller.reset();
        priv->graph.clear();
        priv->graph.reset();
    }

    Globals& AudioEngine::globals() { return world; }

    GraphProcessor&
    AudioEngine::graph()
    {
        return priv->graph;
    }

    Transport*
    AudioEngine::transport()
    {
        return &priv->transport;
    }

}

