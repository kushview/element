/*
    AudioEngine.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.

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

#include "engine/AudioEngine.h"
#include "engine/ClipFactory.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "engine/MidiClipSource.h"
#include "engine/Transport.h"

#include "session/DeviceManager.h"
#include "session/PluginManager.h"

#include "EngineControl.h"
#include "Globals.h"
#include "MediaManager.h"

namespace Element {

    namespace EngineHelpers {
        
        GraphNodePtr createNodeFromValueTreeNode (GraphProcessor& graph, const ValueTree& node,
                                                  PluginManager& plugins)
        {
            /* const bool restoreAudioPatch     = true; // ! filters.contains (Snapshot::filterAudioPatch); */
            const bool restorePluginGain     = true; // ! filters.contains (Snapshot::filterPluginGain);
            const bool restorePluginSettings = true; // ! filters.contains (Snapshot::filterPluginSettings);
            const bool restoreBypassState    = true; // ! filters.contains (Snapshot::filterBypassState);
            const bool restoreWindowLayout   = true; // ! filters.contains (Snapshot::filterWindowLayout);

            ValueTree plugin = node.getChildWithName ("plugin");
            PluginDescription desc;
            desc.pluginFormatName = plugin.getProperty ("format");
            desc.name = plugin.getProperty("name");
            desc.category = plugin.getProperty ("category");
            desc.manufacturerName = plugin.getProperty ("manufacturer");
            desc.version = plugin.getProperty ("version");
            desc.fileOrIdentifier = plugin.getProperty ("file");
            desc.uid = CharacterFunctions::getIntValue<int> (plugin.getProperty ("uid").toString().toUTF8());
            desc.isInstrument = (bool)plugin.getProperty ("isInstrument");
            // TODO: desc.fileTime = plugin.getProperty ("fileTime");
            desc.numInputChannels = (int) plugin.getProperty ("numInputs");
            desc.numOutputChannels = (int) plugin.getProperty ("numOutputs");
            desc.hasSharedContainer = (bool) plugin.getProperty ("isShell");

            String errorMessage;
            Processor* instance = plugins.createPlugin (desc, errorMessage);

            if (instance == nullptr) {
                DBG (errorMessage);
                return nullptr;
            }

            GraphNodePtr nodePtr (graph.addNode (instance, (uint32)(int) node.getProperty ("id")));
            /* TODO: Prevent memory leaks. Unlikely, but the Processor instance here could leak
             memory if the node wasn't created - MRF */

            
            if (nodePtr)
            {
                ValueTree meta = node.getChildWithName ("metadata");
                if (meta.isValid())
                    nodePtr->setMetadata (meta, true);

                if (node.hasProperty("gain") && restorePluginGain) {
                    nodePtr->setGain ((float) node.getProperty("gain"));
                }

                if (plugin.hasProperty ("state") && restorePluginSettings)
                {
                    MemoryBlock m;
                    m.fromBase64Encoding (plugin.getProperty("state").toString());
                    nodePtr->getAudioPluginInstance()->setStateInformation (m.getData(), (int) m.getSize());
                }

                if (plugin.hasProperty("isSuspended") && restoreBypassState)
                    nodePtr->getAudioPluginInstance()->suspendProcessing ((bool) plugin.getProperty ("isSuspended", false));

                // set misc properties from the node property store
                Array<Identifier> ignore;
                if (! restoreWindowLayout) {
                    ignore.add (Identifier("uiLastX"));
                    ignore.add (Identifier("uiLastY"));
                }

                for (int i = 0; i < node.getNumProperties(); ++i)
                    if (! ignore.contains (node.getPropertyName (i)))
                        nodePtr->properties.set (node.getPropertyName(i), node.getProperty (node.getPropertyName(i)));
            }
            else
            {
                DBG ("Could not create plugin");
            }

            return nodePtr;
        }

        ValueTree createValueTreeForNode (GraphNodePtr node, Identifier valueTreeType = "node")
        {
            ValueTree result (valueTreeType);

            result.setProperty ("id", static_cast<int> (node->nodeId), nullptr);
            for (int i = 0; i < node->properties.size(); ++i)
            {
                result.setProperty (node->properties.getName(i),
                                    node->properties.getValueAt(i),
                                    nullptr);
            }

            result.setProperty ("gain", node->getGain(), nullptr);

            if (AudioPluginInstance* plugin = dynamic_cast<AudioPluginInstance*> (node->getAudioPluginInstance()))
            {
                PluginDescription pd;
                plugin->fillInPluginDescription (pd);

                ValueTree p ("plugin");
                p.setProperty (Slugs::name, pd.name, nullptr);
                if (pd.descriptiveName != pd.name)
                    p.setProperty("descriptiveName", pd.descriptiveName, nullptr);

                p.setProperty ("format",       pd.pluginFormatName, nullptr);
                p.setProperty ("category",     pd.category, nullptr);
                p.setProperty ("manufacturer", pd.manufacturerName, nullptr);
                p.setProperty ("version",      pd.version, nullptr);
                p.setProperty ("file",         pd.fileOrIdentifier, nullptr);
                p.setProperty ("uid",          String::toHexString (pd.uid), nullptr);
                p.setProperty ("isInstrument", pd.isInstrument, nullptr);
                p.setProperty ("fileTime",     String::toHexString (pd.lastFileModTime.toMilliseconds()), nullptr);
                p.setProperty ("numInputs",    pd.numInputChannels, nullptr);
                p.setProperty ("numOutputs",   pd.numOutputChannels, nullptr);
                p.setProperty ("isShell",      pd.hasSharedContainer, nullptr);
                p.setProperty ("isSuspended",  plugin->isSuspended(), nullptr);

                MemoryBlock m;
                node->getProcessor()->getStateInformation (m);
                p.setProperty ("state", m.toBase64Encoding(), nullptr);

                result.addChild (p, -1, nullptr);

                ValueTree meta (node->getMetadata().createCopy());
                result.addChild (meta, -1, nullptr);
            }

            return result;
        }
    }

class RootGraph : public GraphProcessor
{
public:
    RootGraph() { }
    ~RootGraph() { }

    void restoreFromValueTree (const ValueTree& data, PluginManager& plugins)
    {
        clear();

        ValueTree arcs  = data.getChildWithName ("arcs");
        ValueTree nodes = data.getChildWithName ("nodes");
        for (int i = 0; i < nodes.getNumChildren(); ++i)
        {
            ValueTree c (nodes.getChild(i));
            if (! c.hasType ("node"))
                continue;
            
            EngineHelpers::createNodeFromValueTreeNode (*this, c, plugins);
        }

        // if (! filters.contains (Snapshot::filterAudioPatch))
        {
            for (int i = 0; i < arcs.getNumChildren(); ++i)
            {
                ValueTree c = arcs.getChild(i);
                if (c.isValid() && c.hasType("arc"))
                {
                    addConnection ((uint32)(int) c.getProperty ("sourceNode"),
                                   (uint32)(int) c.getProperty ("sourcePort"),
                                   (uint32)(int) c.getProperty ("destNode"),
                                   (uint32)(int) c.getProperty ("destPort"));
                }
            }

            removeIllegalConnections();
        }
    }

    ValueTree createValueTree() const
    {
        ValueTree graph (Element::Slugs::graph);

        ValueTree nodes ("nodes");
        for (int i = 0; i < getNumNodes(); ++i)
        {
            GraphNodePtr node = getNode (i);

            AudioPluginInstance* plugin = dynamic_cast <AudioPluginInstance*> (node->getProcessor());
            if (plugin == nullptr)
            {
                jassertfalse;
                return ValueTree::invalid;
            }

            ValueTree n (EngineHelpers::createValueTreeForNode (node));
            if (n.isValid()) nodes.addChild (n, -1, nullptr);
        }
        graph.addChild (nodes, -1, nullptr);

        ValueTree arcs ("arcs");
        for (int i = 0; i < getNumConnections(); ++i)
        {
            const GraphProcessor::Connection* const fc = getConnection(i);
            ValueTree arc ("arc");

            arc.setProperty ("sourceNode", (int) fc->sourceNode, nullptr);
            arc.setProperty ("sourcePort", (int) fc->sourcePort, nullptr);
            arc.setProperty ("destNode",   (int) fc->destNode,   nullptr);
            arc.setProperty ("destPort",   (int) fc->destPort,   nullptr);

            arcs.addChild (arc, -1, nullptr);
        }

        graph.addChild (arcs, -1, nullptr);

        return graph;
    }
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
                                const int numSamples)
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
            const ScopedLock sl2 (processor->getCallbackLock());

            if (processor->isSuspended())
            {
                for (int i = 0; i < numOutputChannels; ++i)
                    zeromem (outputChannelData[i], sizeof (float) * (size_t) numSamples);
            }
            else if (engine->transport()->getRemainingFrames() >= numSamples)
            {
                messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples);
                AudioSampleBuffer buffer (channels, totalNumChans, numSamples);
                processor->processBlock (buffer, incomingMidi);
                if (engine->transport()->isPlaying()) {
                    engine->transport()->advance (numSamples);
                }
            }
            else
            {
                AudioSampleBuffer buffer1 (channels, totalNumChans, 0, remainingFrames);
                messageCollector.removeNextBlockOfMessages (incomingMidi, remainingFrames);
                processor->processBlock (buffer1, incomingMidi);
                incomingMidi.clear();
                if (engine->transport()->isPlaying()) {
                    engine->transport()->advance (remainingFrames);
                }

                AudioSampleBuffer buffer2 (channels, totalNumChans, remainingFrames, numSamples - remainingFrames);
                messageCollector.removeNextBlockOfMessages (incomingMidi, numSamples - remainingFrames);
                processor->processBlock (buffer2, incomingMidi);
                if (engine->transport()->isPlaying()) {
                    engine->transport()->advance (numSamples - remainingFrames);
                }
            }
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

void AudioEngine::activate()
{
    cb->setRootGraph (&graph());
    globals().devices().addMidiInputCallback (String::empty, dynamic_cast<MidiInputCallback*> (&this->callback()));
    Shared<EngineControl> c (controller());

    InternalFormat* fmt = globals().plugins().format<InternalFormat>();
    c->addFilter (fmt->description (InternalFormat::audioInputDevice));
    c->addFilter (fmt->description (InternalFormat::audioOutputDevice));
    c->addFilter (fmt->description (InternalFormat::midiInputDevice));
}

AudioIODeviceCallback&  AudioEngine::callback()              { assert (cb != nullptr); return *cb; }
MidiInputCallback&      AudioEngine::getMidiInputCallback()  { assert (cb != nullptr); return *cb; }

ClipFactory& AudioEngine::clips()
{
    assert (priv->clips != nullptr);
    return *priv->clips;
}

Shared<EngineControl> AudioEngine::controller()
{
    if (priv->controller == nullptr)
        priv->controller = Shared<EngineControl> (new EngineControl (*this));
    return priv->controller;
}

void AudioEngine::deactivate()
{
    cb->setRootGraph (nullptr);
    globals().devices().removeMidiInputCallback (String::empty, dynamic_cast<MidiInputCallback*> (&this->callback()));
    priv->controller.reset();
    priv->graph.clear();
    priv->graph.reset();
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
    return (priv) ? priv->graph.createValueTree() : ValueTree::invalid;
}

void AudioEngine::restoreFromGraphTree (const ValueTree& tree)
{
    if (priv) {
        priv->graph.restoreFromValueTree (tree, world.plugins());
    }
}

}
