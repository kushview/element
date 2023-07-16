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

#include <element/context.hpp>
#include <element/nodefactory.hpp>

#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/AudioProcessorNode.h"
#include "engine/nodes/AudioRouterNode.h"
#include "engine/nodes/MidiChannelSplitterNode.h"
#include "engine/nodes/MidiMonitorNode.h"
#include "engine/nodes/MidiProgramMapNode.h"
#include "engine/nodes/MidiRouterNode.h"
// #include "engine/nodes/MidiSequencerNode.h"
#include "engine/nodes/OSCReceiverNode.h"
#include "engine/nodes/OSCSenderNode.h"
#include "engine/nodes/ScriptNode.h"
#include "engine/graphnode.hpp"

#include "engine/audioprocessorfactory.hpp"
#include "engine/internalformat.hpp"

#include "../nodes/mcu.hpp"

namespace element {

class AudioProcessorFactory::Format : public ElementAudioPluginFormat
{
public:
    Format (Context& ctx) : ElementAudioPluginFormat (ctx) {}
};

AudioProcessorFactory::AudioProcessorFactory (Context& c)
{
    _format = std::make_unique<Format> (c);
}

AudioProcessorFactory::~AudioProcessorFactory() {}

/** Create the instance by ID string. */
Processor* AudioProcessorFactory::create (const String& ID)
{
    double sampleRate = 44100.0;
    int bufferSize = 1024;
    juce::PluginDescription desc;
    desc.pluginFormatName = EL_NODE_FORMAT_NAME;
    desc.fileOrIdentifier = ID;
    if (auto ap = _format->createInstanceFromDescription (desc, sampleRate, bufferSize))
    {
        ap->enableAllBuses(); // TODO pluginmanager does this as well.
        return NodeFactory::wrap (ap.release());
    }
    return nullptr;
}

/** return a list of types contained in this provider. */
StringArray AudioProcessorFactory::findTypes()
{
    return _format->searchPathsForPlugins ({}, true, true);
}

//==============================================================================
template <class NT>
struct SingleNodeProvider : public NodeProvider
{
    const String ID;
    const String UI;

    SingleNodeProvider() = delete;
    SingleNodeProvider (const String& inID)
        : ID (inID) {}
    ~SingleNodeProvider() = default;

    StringArray findTypes() override
    {
        return StringArray (ID);
    }

    Processor* create (const String& nodeId) override
    {
        return (this->ID == nodeId) ? new NT() : nullptr;
    }
};

class NodeFactory::Impl
{
public:
    Impl (NodeFactory& f) : factory (f)
    {
    }

    ~Impl()
    {
        providers.clearQuick (true);
        knownIDs.clearQuick();
    }

private:
    friend class NodeFactory;
    OwnedArray<NodeProvider> providers;
    NodeFactory& factory;
    StringArray knownIDs;
    StringArray denyIDs;
};

NodeFactory::NodeFactory()
{
    impl = std::make_unique<Impl> (*this);
    add (new SingleNodeProvider<AudioRouterNode> (EL_NODE_ID_AUDIO_ROUTER));
    add (new SingleNodeProvider<MidiChannelSplitterNode> (EL_NODE_ID_MIDI_CHANNEL_SPLITTER));
    add (new SingleNodeProvider<MidiMonitorNode> (EL_NODE_ID_MIDI_MONITOR));
    add (new SingleNodeProvider<MidiProgramMapNode> (EL_NODE_ID_MIDI_PROGRAM_MAP));
    add (new SingleNodeProvider<MidiRouterNode> (EL_NODE_ID_MIDI_ROUTER));
    add (new SingleNodeProvider<OSCSenderNode> (EL_NODE_ID_OSC_SENDER));
    add (new SingleNodeProvider<OSCReceiverNode> (EL_NODE_ID_OSC_RECEIVER));
    add (new SingleNodeProvider<ScriptNode> (EL_NODE_ID_SCRIPT));
    add (new SingleNodeProvider<GraphNode> (EL_NODE_ID_GRAPH));
    add (new SingleNodeProvider<MackieControlUniversal> ("el.MCU"));
}

NodeFactory::~NodeFactory()
{
    impl.reset();
}

//==============================================================================
void NodeFactory::getPluginDescriptions (OwnedArray<PluginDescription>& out, const String& ID, bool includeHidden)
{
    auto& denyIDs (impl->denyIDs);
    if (! includeHidden && denyIDs.contains (ID))
        return;

    for (auto* f : impl->providers)
    {
        if (f->format() != EL_NODE_FORMAT_NAME)
            continue;

        if (ProcessorPtr ptr = f->create (ID))
        {
            auto* desc = out.add (new PluginDescription());
            ptr->getPluginDescription (*desc);
            break;
        }
    }
}

/** Fill a list of plugin descriptions. public */
void NodeFactory::getPluginDescriptions (OwnedArray<PluginDescription>& out,
                                         const String& format,
                                         const String& identifier,
                                         bool includeHidden)
{
    if (format == EL_NODE_FORMAT_NAME)
    {
        getPluginDescriptions (out, identifier, includeHidden);
        return;
    }

    for (auto* f : impl->providers)
    {
        if (f->format() != format)
            continue;

        if (ProcessorPtr ptr = f->create (identifier))
        {
            auto* desc = out.add (new PluginDescription());
            ptr->getPluginDescription (*desc);
            break;
        }
    }
}

const StringArray& NodeFactory::knownIDs() const noexcept { return impl->knownIDs; }

//==============================================================================
NodeFactory& NodeFactory::add (NodeProvider* f)
{
    auto& providers (impl->providers);
    providers.add (f);

    auto& denyIDs (impl->denyIDs);
    denyIDs.addArray (f->getHiddenTypes());
    denyIDs.removeDuplicates (true);
    denyIDs.removeEmptyStrings();

    auto& knownIDs (impl->knownIDs);
    knownIDs.addArray (f->findTypes());
    knownIDs.removeDuplicates (true);
    knownIDs.removeEmptyStrings();
    return *this;
}

//==============================================================================
void NodeFactory::hideType (const String& tp)
{
    auto& denyIDs (impl->denyIDs);
    denyIDs.addIfNotAlreadyThere (tp);
}

void NodeFactory::hideAllTypes()
{
    auto& denyIDs (impl->denyIDs);
    for (const auto& tp : impl->knownIDs)
        denyIDs.add (tp);

    // TODO: Nodes backed by juce::AudioProcessor
    denyIDs.add (EL_NODE_ID_ALLPASS_FILTER);
    denyIDs.add (EL_NODE_ID_AUDIO_FILE_PLAYER);
    denyIDs.add (EL_NODE_ID_AUDIO_MIXER);
    denyIDs.add (EL_NODE_ID_CHANNELIZE);
    denyIDs.add (EL_NODE_ID_COMB_FILTER);
    denyIDs.add (EL_NODE_ID_COMPRESSOR);
    denyIDs.add (EL_NODE_ID_EQ_FILTER);
    denyIDs.add (EL_NODE_ID_FREQ_SPLITTER);
    denyIDs.add (EL_NODE_ID_MEDIA_PLAYER);
    denyIDs.add (EL_NODE_ID_MIDI_CHANNEL_MAP);
    denyIDs.add (EL_NODE_ID_MIDI_INPUT_DEVICE);
    denyIDs.add (EL_NODE_ID_MIDI_OUTPUT_DEVICE);
    denyIDs.add (EL_NODE_ID_PLACEHOLDER);
    denyIDs.add (EL_NODE_ID_REVERB);
    denyIDs.add (EL_NODE_ID_WET_DRY);
    denyIDs.add (EL_NODE_ID_VOLUME);
    // end audio procesor nodes

    denyIDs.removeDuplicates (false);
    denyIDs.removeEmptyStrings();
}

bool NodeFactory::isTypeHidden (const String& tp) const noexcept
{
    auto& denyIDs (impl->denyIDs);
    return denyIDs.contains (tp);
}

void NodeFactory::removeHiddenType (const String& tp)
{
    auto& denyIDs (impl->denyIDs);
    auto idx = denyIDs.indexOf (tp);
    if (idx >= 0)
        denyIDs.remove (idx);
}

//==============================================================================
Processor* NodeFactory::instantiate (const PluginDescription& desc)
{
    return instantiate (desc.fileOrIdentifier);
}

Processor* NodeFactory::instantiate (const String& identifier)
{
    auto& providers (impl->providers);
    Processor* node = nullptr;
    for (const auto& f : providers)
        if (auto* const n = f->create (identifier))
        {
            node = n;
            break;
        }

    if (node)
    {
        // node->init();
    }

    return node;
}

Processor* NodeFactory::wrap (AudioProcessor* processor)
{
    jassert (processor);
    auto node = std::make_unique<AudioProcessorNode> (0, processor);

    if (node)
    {
        // noop.
    }

    return node.release();
}

const OwnedArray<NodeProvider>& NodeFactory::providers() const noexcept { return impl->providers; }

} // namespace element
