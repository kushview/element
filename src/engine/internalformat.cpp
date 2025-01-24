// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ElementApp.h"

#include <element/audioengine.hpp>
#include <element/context.hpp>
#include <element/session.hpp>
#include <element/plugins.hpp>
#include <element/nodefactory.hpp>

#include "engine/midiengine.hpp"

#include "nodes/allpassfilter.hpp"
#include "nodes/audiofileplayer.hpp"
#include "nodes/audiomixer.hpp"
#include "nodes/channelize.hpp"
#include "nodes/combfilter.hpp"
#include "nodes/compressor.hpp"
#include "nodes/eqfilter.hpp"
#include "nodes/freqsplitter.hpp"
#include "nodes/mediaplayer.hpp"
#include "nodes/midichannelmap.hpp"
#include "nodes/mididevice.hpp"
#include "nodes/midisetlist.hpp"
#include "nodes/placeholder.hpp"
#include "nodes/everb.hpp"
#include "nodes/volume.hpp"
#include "nodes/wetdry.hpp"
#include "nodes/midiprogrammap.hpp"

#include "engine/graphnode.hpp"
#include "engine/internalformat.hpp"
#include "engine/ionode.hpp"

namespace element {

using namespace juce;

//==============================================================================
ElementAudioPluginFormat::ElementAudioPluginFormat (Context& c)
    : _context (c) {}

void ElementAudioPluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& ds, const String& fileOrId)
{
    auto& factory = _context.plugins().getNodeFactory();
    if (factory.isTypeHidden (fileOrId))
    {
        return;
    }

    if (fileOrId == "element.comb")
    {
        auto* desc = ds.add (new PluginDescription());
        desc->pluginFormatName = getName();
        desc->name = "Comb Filter (mono)";
        desc->manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc->category = "Effect";
        desc->fileOrIdentifier = fileOrId + ".mono";
        desc->numInputChannels = 1;
        desc->numOutputChannels = 1;

        desc = ds.add (new PluginDescription (*desc));
        desc->name = "Comb Filter (stereo)";
        desc->fileOrIdentifier = fileOrId + ".stereo";
        desc->numInputChannels = 2;
        desc->numOutputChannels = 2;
    }
    else if (fileOrId == "element.allPass")
    {
        auto* desc = ds.add (new PluginDescription());
        desc->pluginFormatName = getName();
        desc->name = "AllPass Filter (mono)";
        desc->manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc->category = "Effect";
        desc->fileOrIdentifier = fileOrId + ".mono";
        desc->numInputChannels = 1;
        desc->numOutputChannels = 1;

        desc = ds.add (new PluginDescription (*desc));
        desc->name = "AllPass Filter (stereo)";
        desc->fileOrIdentifier = fileOrId + ".stereo";
        desc->numInputChannels = 2;
        desc->numOutputChannels = 2;
    }
    else if (fileOrId == "element.volume")
    {
        auto* desc = ds.add (new PluginDescription());
        desc->pluginFormatName = getName();
        desc->name = "Volume (mono)";
        desc->manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc->category = "Effect";
        desc->fileOrIdentifier = fileOrId + ".mono";
        desc->numInputChannels = 1;
        desc->numOutputChannels = 1;

        desc = ds.add (new PluginDescription (*desc));
        desc->name = "Volume (stereo)";
        desc->fileOrIdentifier = fileOrId + ".stereo";
        desc->numInputChannels = 2;
        desc->numOutputChannels = 2;
    }
    else if (fileOrId == EL_NODE_ID_WET_DRY)
    {
        auto* desc = ds.add (new PluginDescription());
        WetDryProcessor wetDry;
        wetDry.fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_REVERB)
    {
        auto* desc = ds.add (new PluginDescription());
        ReverbProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_EQ_FILTER)
    {
        auto* desc = ds.add (new PluginDescription());
        EQFilterProcessor (2).fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_FREQ_SPLITTER)
    {
        auto* desc = ds.add (new PluginDescription());
        FreqSplitterProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_COMPRESSOR)
    {
        auto* desc = ds.add (new PluginDescription());
        CompressorProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_AUDIO_MIXER)
    {
        auto* const desc = ds.add (new PluginDescription());
        AudioMixerProcessor (4).fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_CHANNELIZE)
    {
        auto* const desc = ds.add (new PluginDescription());
        ChannelizeProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_MIDI_CHANNEL_MAP)
    {
        auto* const desc = ds.add (new PluginDescription());
        MidiChannelMapProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_AUDIO_FILE_PLAYER)
    {
        auto* const desc = ds.add (new PluginDescription());
        AudioFilePlayerNode().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_MEDIA_PLAYER)
    {
        auto* const desc = ds.add (new PluginDescription());
        MediaPlayerProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_PLACEHOLDER)
    {
        auto* const desc = ds.add (new PluginDescription());
        PlaceholderProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_MIDI_INPUT_DEVICE)
    {
        auto* const desc = ds.add (new PluginDescription());
        MidiDeviceProcessor (true, _context.midi()).fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_NODE_ID_MIDI_OUTPUT_DEVICE)
    {
        auto* const desc = ds.add (new PluginDescription());
        MidiDeviceProcessor (false, _context.midi()).fillInPluginDescription (*desc);
    }
}

StringArray ElementAudioPluginFormat::searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/)
{
    StringArray results;
    results.add (EL_NODE_ID_COMB_FILTER);
    results.add (EL_NODE_ID_COMPRESSOR);
    results.add (EL_NODE_ID_EQ_FILTER);
    results.add (EL_NODE_ID_FREQ_SPLITTER);
    results.add (EL_NODE_ID_ALLPASS_FILTER);
    results.add (EL_NODE_ID_VOLUME);
    results.add (EL_NODE_ID_WET_DRY);
    results.add (EL_NODE_ID_REVERB);
    results.add (EL_NODE_ID_AUDIO_MIXER);
    results.add (EL_NODE_ID_CHANNELIZE);
    results.add (EL_NODE_ID_MEDIA_PLAYER);
    results.add (EL_NODE_ID_MIDI_CHANNEL_MAP);
    results.add (EL_NODE_ID_AUDIO_FILE_PLAYER);
    results.add (EL_NODE_ID_PLACEHOLDER);
    results.add (EL_NODE_ID_MIDI_INPUT_DEVICE);
    results.add (EL_NODE_ID_MIDI_OUTPUT_DEVICE);
    return results;
}

AudioPluginInstance* ElementAudioPluginFormat::instantiatePlugin (const PluginDescription& desc,
                                                                  double sampleRate,
                                                                  int blockSize)
{
    std::unique_ptr<AudioPluginInstance> base;

    if (desc.fileOrIdentifier == "element.comb.mono" || desc.fileOrIdentifier == EL_NODE_ID_COMB_FILTER)
        base = std::make_unique<CombFilterProcessor> (false);
    else if (desc.fileOrIdentifier == "element.comb.stereo")
        base = std::make_unique<CombFilterProcessor> (true);

    else if (desc.fileOrIdentifier == "element.allPass.mono" || desc.fileOrIdentifier == EL_NODE_ID_ALLPASS_FILTER)
        base = std::make_unique<AllPassFilterProcessor> (false);
    else if (desc.fileOrIdentifier == "element.allPass.stereo")
        base = std::make_unique<AllPassFilterProcessor> (true);

    else if (desc.fileOrIdentifier == "element.volume.mono")
        base = std::make_unique<VolumeProcessor> (-30.0, 12.0, false);
    else if (desc.fileOrIdentifier == "element.volume.stereo" || desc.fileOrIdentifier == EL_NODE_ID_VOLUME)
        base = std::make_unique<VolumeProcessor> (-30.0, 12.0, true);

    else if (desc.fileOrIdentifier == EL_NODE_ID_WET_DRY)
        base = std::make_unique<WetDryProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_REVERB)
        base = std::make_unique<ReverbProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_EQ_FILTER)
        base = std::make_unique<EQFilterProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_FREQ_SPLITTER)
        base = std::make_unique<FreqSplitterProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_COMPRESSOR)
        base = std::make_unique<CompressorProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_AUDIO_MIXER)
        base = std::make_unique<AudioMixerProcessor> (4, sampleRate, blockSize);
    else if (desc.fileOrIdentifier == EL_NODE_ID_CHANNELIZE)
        base = std::make_unique<ChannelizeProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_MIDI_CHANNEL_MAP)
        base = std::make_unique<MidiChannelMapProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_AUDIO_FILE_PLAYER)
        base = std::make_unique<AudioFilePlayerNode>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_MEDIA_PLAYER)
        base = std::make_unique<MediaPlayerProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_PLACEHOLDER)
        base = std::make_unique<PlaceholderProcessor>();
    else if (desc.fileOrIdentifier == EL_NODE_ID_MIDI_INPUT_DEVICE)
        base = std::make_unique<MidiDeviceProcessor> (true, _context.midi());
    else if (desc.fileOrIdentifier == EL_NODE_ID_MIDI_OUTPUT_DEVICE)
        base = std::make_unique<MidiDeviceProcessor> (false, _context.midi());

    return base != nullptr ? base.release() : nullptr;
}

void ElementAudioPluginFormat::createPluginInstance (const PluginDescription& d,
                                                     double initialSampleRate,
                                                     int initialBufferSize,
                                                     PluginCreationCallback callback)
{
    if (auto* i = instantiatePlugin (d, initialSampleRate, initialBufferSize))
        callback (std::unique_ptr<AudioPluginInstance> (i), String());
    else
        callback (nullptr, String());
}

bool ElementAudioPluginFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
{
    return false;
}

//==============================================================================
InternalNodes::InternalNodes (Context& c)
    : _context (c) {}

Processor* InternalNodes::create (const String& ID)
{
    if (ID == EL_NODE_ID_GRAPH)
    {
        return new GraphNode (_context);
    }
    else if (ID == EL_NODE_ID_MIDI_SET_LIST)
    {
        return new MidiSetListProcessor (_context);
    }

    return nullptr;
}

StringArray InternalNodes::findTypes (const juce::FileSearchPath&, bool, bool)
{
    return { EL_NODE_ID_GRAPH, EL_NODE_ID_MIDI_SET_LIST };
}

StringArray InternalNodes::getHiddenTypes() { return {}; }

} // namespace element
