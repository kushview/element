// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/context.hpp>
#include <element/nodefactory.hpp>

#include "nodes/baseprocessor.hpp"
#include "nodes/audioprocessor.hpp"
#include "nodes/audiorouter.hpp"
#include "nodes/midichannelsplitter.hpp"
#include "nodes/midimonitor.hpp"
#include "nodes/midiprogrammap.hpp"
#include "nodes/midirouter.hpp"
// #include "nodes/MidiSequencerNode.h"
#include "nodes/oscreceiver.hpp"
#include "nodes/oscsender.hpp"
#include "nodes/scriptnode.hpp"
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
StringArray AudioProcessorFactory::findTypes (const juce::FileSearchPath& f, bool r, bool a)
{
    return _format->searchPathsForPlugins (f, r, a);
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

    String format() const override { return EL_NODE_FORMAT_NAME; }
    StringArray findTypes (const juce::FileSearchPath&, bool, bool) override
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
    Impl (NodeFactory& f) : factory (f) {}

    ~Impl()
    {
        providers.clearQuick (true);
        knownIDs.clearQuick();
    }

private:
    friend class NodeFactory;
    OwnedArray<NodeProvider> providers;
    [[maybe_unused]] NodeFactory& factory;
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
    add (new SingleNodeProvider<MackieControlUniversal> ("el.MCU"));
#if ! JUCE_DEBUG
    hideType ("el.MCU");
#endif
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
    {
        return;
    }

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
    knownIDs.addArray (f->findTypes ({}, true, false));
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
    const auto ID = desc.fileOrIdentifier;
    if (desc.pluginFormatName == EL_NODE_FORMAT_NAME)
        return instantiate (ID);

    auto& providers (impl->providers);
    Processor* node = nullptr;
    for (const auto& f : providers)
    {
        if (f->format() != desc.pluginFormatName)
            continue;

        if (auto* const n = f->create (ID))
        {
            node = n;
            break;
        }
    }

    return node;
}

Processor* NodeFactory::instantiate (const String& identifier)
{
    auto& providers (impl->providers);
    Processor* node = nullptr;
    for (const auto& f : providers)
    {
        if (f->format() != EL_NODE_FORMAT_NAME)
            continue;

        if (auto* const n = f->create (identifier))
        {
            node = n;
            break;
        }
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
