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

#include "ElementApp.h"

#include <element/audioengine.hpp>
#include <element/context.hpp>
#include <element/session.hpp>

#include "engine/midiengine.hpp"

#include "engine/nodes/AllPassFilterNode.h"
#include "engine/nodes/AudioFilePlayerNode.h"
#include "engine/nodes/AudioMixerProcessor.h"
#include "engine/nodes/ChannelizeProcessor.h"
#include "engine/nodes/CombFilterProcessor.h"
#include "engine/nodes/CompressorProcessor.h"
#include "engine/nodes/EQFilterProcessor.h"
#include "engine/nodes/FreqSplitterProcessor.h"
#include "engine/nodes/MediaPlayerProcessor.h"
#include "engine/nodes/MidiChannelMapProcessor.h"
#include "engine/nodes/MidiDeviceProcessor.h"
#include "engine/nodes/PlaceholderProcessor.h"
#include "engine/nodes/ReverbProcessor.h"
#include "engine/nodes/VolumeProcessor.h"
#include "engine/nodes/WetDryProcessor.h"

#include "engine/internalformat.hpp"
#include "engine/ionode.hpp"

namespace element {

InternalFormat::InternalFormat (Context& ctx)
    : context (ctx)
{
    {
        PlaceholderProcessor p;
        p.fillInPluginDescription (placeholderDesc);
    }
    {
        MidiDeviceProcessor in (true, context.getMidiEngine());
        in.fillInPluginDescription (midiInputDeviceDesc);
        MidiDeviceProcessor out (false, context.getMidiEngine());
        out.fillInPluginDescription (midiOutputDeviceDesc);
    }
}

AudioPluginInstance* InternalFormat::instantiatePlugin (const PluginDescription& desc, double, int)
{
    if (desc.fileOrIdentifier == EL_INTERNAL_ID_MIDI_INPUT_DEVICE)
    {
        return new MidiDeviceProcessor (true, context.getMidiEngine());
    }
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MIDI_OUTPUT_DEVICE)
    {
        return new MidiDeviceProcessor (false, context.getMidiEngine());
    }
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_PLACEHOLDER)
    {
        return new PlaceholderProcessor();
    }

    return nullptr;
}

void InternalFormat::getAllTypes (OwnedArray<PluginDescription>& results)
{
    results.add (new PluginDescription (placeholderDesc));
    results.add (new PluginDescription (midiInputDeviceDesc));
    results.add (new PluginDescription (midiOutputDeviceDesc));
}

void InternalFormat::createPluginInstance (const PluginDescription& d, double initialSampleRate, int initialBufferSize, PluginCreationCallback callback)
{
    if (auto* i = instantiatePlugin (d, initialSampleRate, initialBufferSize))
    {
        callback (std::unique_ptr<AudioPluginInstance> (i), String());
    }
    else
    {
        callback (nullptr, String ("Could not instantiate ") + d.name);
    }
}

bool InternalFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
{
    return false;
}

// MARK: Element Format
ElementAudioPluginFormat::ElementAudioPluginFormat (Context& g)
    : world (g) {}

void ElementAudioPluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& ds, const String& fileOrId)
{
    if (fileOrId == "element.comb")
    {
        auto* desc = ds.add (new PluginDescription());
        desc->pluginFormatName = getName();
        desc->name = "Comb Filter (mono)";
        desc->manufacturerName = "Element";
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
        desc->manufacturerName = "Element";
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
        desc->manufacturerName = "Element";
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
    else if (fileOrId == EL_INTERNAL_ID_WET_DRY)
    {
        auto* desc = ds.add (new PluginDescription());
        WetDryProcessor wetDry;
        wetDry.fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_REVERB)
    {
        auto* desc = ds.add (new PluginDescription());
        ReverbProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_EQ_FILTER)
    {
        auto* desc = ds.add (new PluginDescription());
        EQFilterProcessor (2).fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_FREQ_SPLITTER)
    {
        auto* desc = ds.add (new PluginDescription());
        FreqSplitterProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_COMPRESSOR)
    {
        auto* desc = ds.add (new PluginDescription());
        CompressorProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_AUDIO_MIXER)
    {
        auto* const desc = ds.add (new PluginDescription());
        AudioMixerProcessor (4).fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_CHANNELIZE)
    {
        auto* const desc = ds.add (new PluginDescription());
        ChannelizeProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_MIDI_CHANNEL_MAP)
    {
        auto* const desc = ds.add (new PluginDescription());
        MidiChannelMapProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_AUDIO_FILE_PLAYER)
    {
        auto* const desc = ds.add (new PluginDescription());
        AudioFilePlayerNode().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_MEDIA_PLAYER)
    {
        auto* const desc = ds.add (new PluginDescription());
        MediaPlayerProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_PLACEHOLDER)
    {
        auto* const desc = ds.add (new PluginDescription());
        PlaceholderProcessor().fillInPluginDescription (*desc);
    }
}

StringArray ElementAudioPluginFormat::searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/)
{
    StringArray results;
    results.add (EL_INTERNAL_ID_COMB_FILTER);
    results.add (EL_INTERNAL_ID_COMPRESSOR);
    results.add (EL_INTERNAL_ID_EQ_FILTER);
    results.add (EL_INTERNAL_ID_FREQ_SPLITTER);
    results.add (EL_INTERNAL_ID_ALLPASS_FILTER);
    results.add (EL_INTERNAL_ID_VOLUME);
    results.add (EL_INTERNAL_ID_WET_DRY);
    results.add (EL_INTERNAL_ID_REVERB);
    results.add (EL_INTERNAL_ID_AUDIO_MIXER);
    results.add (EL_INTERNAL_ID_CHANNELIZE);
    results.add (EL_INTERNAL_ID_MEDIA_PLAYER);
    results.add (EL_INTERNAL_ID_MIDI_CHANNEL_MAP);
    results.add (EL_INTERNAL_ID_AUDIO_FILE_PLAYER);
    results.add (EL_INTERNAL_ID_PLACEHOLDER);
    return results;
}

AudioPluginInstance* ElementAudioPluginFormat::instantiatePlugin (const PluginDescription& desc,
                                                                  double sampleRate,
                                                                  int blockSize)
{
    ScopedPointer<AudioPluginInstance> base;

    if (desc.fileOrIdentifier == "element.comb.mono")
        base = new CombFilterProcessor (false);
    else if (desc.fileOrIdentifier == "element.comb.stereo")
        base = new CombFilterProcessor (true);
    else if (desc.fileOrIdentifier == "element.allPass.mono")
        base = new AllPassFilterProcessor (false);
    else if (desc.fileOrIdentifier == "element.allPass.stereo")
        base = new AllPassFilterProcessor (true);
    else if (desc.fileOrIdentifier == "element.volume.mono")
        base = new VolumeProcessor (-30.0, 12.0, false);
    else if (desc.fileOrIdentifier == "element.volume.stereo")
        base = new VolumeProcessor (-30.0, 12.0, true);
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_WET_DRY)
        base = new WetDryProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_REVERB)
        base = new ReverbProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_EQ_FILTER)
        base = new EQFilterProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_FREQ_SPLITTER)
        base = new FreqSplitterProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_COMPRESSOR)
        base = new CompressorProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_AUDIO_MIXER)
        base = new AudioMixerProcessor (4, sampleRate, blockSize);
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_CHANNELIZE)
        base = new ChannelizeProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MIDI_CHANNEL_MAP)
        base = new MidiChannelMapProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_AUDIO_FILE_PLAYER)
        base = new AudioFilePlayerNode();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MEDIA_PLAYER)
        base = new MediaPlayerProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_PLACEHOLDER)
        base = new PlaceholderProcessor();

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

} // namespace element
