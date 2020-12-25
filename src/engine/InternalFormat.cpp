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

#include "engine/nodes/AllPassFilterNode.h"
#include "engine/nodes/AudioFilePlayerNode.h"
#include "engine/nodes/AudioMixerProcessor.h"
#include "engine/nodes/ChannelizeProcessor.h"
#include "engine/nodes/CombFilterProcessor.h"
#include "engine/nodes/CompressorProcessor.h"
#include "engine/nodes/EQFilterProcessor.h"
#include "engine/nodes/FreqSplitterProcessor.h"
#include "engine/nodes/LuaNode.h"
#include "engine/nodes/MediaPlayerProcessor.h"
#include "engine/nodes/MidiChannelMapProcessor.h"
#include "engine/nodes/MidiChannelSplitterNode.h"
#include "engine/nodes/MidiDeviceProcessor.h"
#include "engine/nodes/MidiMonitorNode.h"
#include "engine/nodes/MidiRouterNode.h"
#include "engine/nodes/PlaceholderProcessor.h"
#include "engine/nodes/OSCReceiverNode.h"
#include "engine/nodes/OSCSenderNode.h"
#include "engine/nodes/ReverbProcessor.h"
#include "engine/nodes/ScriptNode.h"
#include "engine/nodes/SubGraphProcessor.h"
#include "engine/nodes/VolumeProcessor.h"
#include "engine/nodes/WetDryProcessor.h"
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"

#include "session/Session.h"
#include "../experimental/MidiSequenceProcessor.h"

#include "Globals.h"

namespace Element {

typedef GraphProcessor::AudioGraphIOProcessor IOP;

InternalFormat::InternalFormat (AudioEngine& e, MidiEngine& me)
    : engine (e), midi (me)
{
    {
        IOP p (IOP::audioOutputNode);
        p.fillInPluginDescription (audioOutDesc);
    }

    {
        IOP p (IOP::audioInputNode);
        p.fillInPluginDescription (audioInDesc);
    }

    {
        IOP p (IOP::midiOutputNode);
        p.fillInPluginDescription (midiOutDesc);
    }
    
    {
        IOP p (IOP::midiInputNode);
        p.fillInPluginDescription (midiInDesc);
    }
    #if defined (EL_PRO) || defined (EL_SOLO)
    {
        PlaceholderProcessor p;
        p.fillInPluginDescription (placeholderDesc);
    }
    {
        MidiDeviceProcessor in (true, midi);
        in.fillInPluginDescription (midiInputDeviceDesc);
        MidiDeviceProcessor out (false, midi);
        out.fillInPluginDescription (midiOutputDeviceDesc);
    }
    #endif
}

AudioPluginInstance* InternalFormat::instantiatePlugin (const PluginDescription& desc, double, int)
{
    if (desc.fileOrIdentifier == audioOutDesc.fileOrIdentifier)
    {
        return new IOP (IOP::audioOutputNode);
    }
    else if (desc.fileOrIdentifier == audioInDesc.fileOrIdentifier)
    {
        return new IOP (IOP::audioInputNode);
    }
    else if (desc.fileOrIdentifier == midiInDesc.fileOrIdentifier)
    {
        return new IOP (IOP::midiInputNode);
    }
    else if (desc.fileOrIdentifier == midiOutDesc.fileOrIdentifier)
    {
        return new IOP (IOP::midiOutputNode);
    }

   #if defined (EL_PRO) || defined (EL_SOLO)
    else if (desc.fileOrIdentifier == "element.midiInputDevice")
    {
        return new MidiDeviceProcessor (true, midi);
    }
    else if (desc.fileOrIdentifier == "element.midiOutputDevice")
    {
        return new MidiDeviceProcessor (false, midi);
    }
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_PLACEHOLDER)
    {
        return new PlaceholderProcessor();
    }
   #endif // EL_FREE

    return nullptr;
}

const PluginDescription* InternalFormat::description (const InternalFormat::ID type)
{
    switch (type)
    {
        case audioInputDevice:      return &audioInDesc;
        case audioOutputDevice:     return &audioOutDesc;
        case midiInputDevice:       return &midiInDesc;
        case midiOutputDevice:      return &midiOutDesc;
        case samplerProcessor:      return &samplerDesc;
        case sequenceProcessor:     return &sequencerDesc;
        case patternProcessor:      return &patternDesc;
        case midiSequence:          return &metroDesc;
        default:                    break;
    }

    return nullptr;
}

void InternalFormat::getAllTypes (OwnedArray <PluginDescription>& results)
{
    for (int i = 0; i < (int) audioOutputPort; ++i)
        results.add (new PluginDescription (*description ((InternalFormat::ID) i)));
    results.add (new PluginDescription (placeholderDesc));
    results.add (new PluginDescription (midiInputDeviceDesc));
    results.add (new PluginDescription (midiOutputDeviceDesc));
}

void InternalFormat::createPluginInstance (const PluginDescription& d, double initialSampleRate,
                                           int initialBufferSize,
                                           PluginCreationCallback callback)
{
    if (auto* i = instantiatePlugin (d, initialSampleRate, initialBufferSize))
        callback (std::unique_ptr<AudioPluginInstance> (i), String());
}

bool InternalFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
{
    return false;
}


// MARK: Element Format

ElementAudioPluginFormat::ElementAudioPluginFormat (Globals& g)
    : world (g) { }

void ElementAudioPluginFormat::findAllTypesForFile (OwnedArray <PluginDescription>& ds, const String& fileOrId)
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
        EQFilterProcessor(2).fillInPluginDescription (*desc);
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

   #if defined (EL_PRO)
    else if (fileOrId == EL_INTERNAL_ID_GRAPH)
    {
        auto* const desc = ds.add (new PluginDescription());
        SubGraphProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_AUDIO_MIXER)
    {
        auto* const desc = ds.add (new PluginDescription());
        AudioMixerProcessor(4).fillInPluginDescription (*desc);
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
    else if (fileOrId == EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER)
    {
        auto* const desc = ds.add (new PluginDescription());
        desc->fileOrIdentifier   = EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER;
        desc->uid                = EL_INTERNAL_UID_MIDI_CHANNEL_SPLITTER;
        desc->name               = "MIDI Channel Splitter";
        desc->descriptiveName    = "MIDI Channel Splitter";
        desc->numInputChannels   = 0;
        desc->numOutputChannels  = 0;
        desc->hasSharedContainer = false;
        desc->isInstrument       = false;
        desc->manufacturerName   = "Element";
        desc->pluginFormatName   = "Element";
        desc->version            = "1.0.0";
    }
   #endif

   #if defined (EL_SOLO) || defined (EL_PRO)
    else if (fileOrId == EL_INTERNAL_ID_AUDIO_FILE_PLAYER)
    {
        auto* const desc = ds.add (new PluginDescription());
        AudioFilePlayerNode().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_AUDIO_ROUTER)
    {
        auto* const desc = ds.add (new PluginDescription());
        desc->fileOrIdentifier   = EL_INTERNAL_ID_AUDIO_ROUTER;
        desc->uid                = EL_INTERNAL_UID_AUDIO_ROUTER;
        desc->name               = "Audio Router";
        desc->descriptiveName    = "An Audio Patch Grid";
        desc->numInputChannels   = 4;
        desc->numOutputChannels  = 4;
        desc->hasSharedContainer = false;
        desc->isInstrument       = false;
        desc->manufacturerName   = "Element";
        desc->pluginFormatName   = "Element";
        desc->version            = "1.0.0";
    }
    else if (fileOrId == EL_INTERNAL_ID_MIDI_ROUTER)
    {
        auto* const desc = ds.add (new PluginDescription());
        MidiRouterNode(4,4).getPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_MEDIA_PLAYER)
    {
        auto* const desc = ds.add (new PluginDescription());
        MediaPlayerProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_MIDI_PROGRAM_MAP)
    {
        auto* const desc = ds.add (new PluginDescription());
        desc->fileOrIdentifier   = EL_INTERNAL_ID_MIDI_PROGRAM_MAP;
        desc->uid                = EL_INTERNAL_UID_MIDI_PROGRAM_MAP;
        desc->name               = "MIDI Program Map";
        desc->descriptiveName    = "Filter MIDI Program Changes";
        desc->numInputChannels   = 0;
        desc->numOutputChannels  = 0;
        desc->hasSharedContainer = false;
        desc->isInstrument       = false;
        desc->manufacturerName   = "Element";
        desc->pluginFormatName   = "Element";
        desc->version            = "1.0.0";
    }
    else if (fileOrId == EL_INTERNAL_ID_PLACEHOLDER)
    {
        auto* const desc = ds.add (new PluginDescription());
        PlaceholderProcessor().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_MIDI_MONITOR)
    {
        auto* const desc = ds.add (new PluginDescription());
        MidiMonitorNode().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_OSC_RECEIVER)
    {
        auto* const desc = ds.add (new PluginDescription());
        OSCReceiverNode().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_OSC_SENDER)
    {
        auto* const desc = ds.add (new PluginDescription());
        OSCSenderNode().fillInPluginDescription (*desc);
    }
    else if (fileOrId == EL_INTERNAL_ID_LUA)
    {
       #if EL_USE_LUA
        auto* const desc = ds.add (new PluginDescription());
        LuaNode().fillInPluginDescription (*desc);
       #endif
    }
    else if (fileOrId == EL_INTERNAL_ID_SCRIPT)
    {
       #if EL_USE_LUA
        auto* const desc = ds.add (new PluginDescription());
        ScriptNode().fillInPluginDescription (*desc);
       #endif
    }
   #endif
}

StringArray ElementAudioPluginFormat::searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/)
{
    StringArray results;
    results.add (EL_INTERNAL_ID_COMB_FILTER);
    results.add (EL_INTERNAL_ID_COMPRESSOR);
    results.add (EL_INTERNAL_ID_EQ_FILTER);
    results.add (EL_INTERNAL_ID_FREQ_SPLITTER);
    results.add ("element.allPass");
    results.add ("element.volume");
    results.add (EL_INTERNAL_ID_WET_DRY);
    results.add (EL_INTERNAL_ID_REVERB);

   #if defined EL_PRO
    results.add (EL_INTERNAL_ID_AUDIO_MIXER);
    results.add (EL_INTERNAL_ID_CHANNELIZE);
    results.add (EL_INTERNAL_ID_MEDIA_PLAYER);
    results.add (EL_INTERNAL_ID_MIDI_CHANNEL_MAP);
    results.add (EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER);
    results.add (EL_INTERNAL_ID_GRAPH);
   #if EL_USE_MIDI_SEQUENCER
    results.add (EL_INTERNAL_ID_MIDI_SEQUENCER);
   #endif
   #endif

   #if defined (EL_SOLO) || defined (EL_PRO)
    results.add (EL_INTERNAL_ID_AUDIO_FILE_PLAYER);
    results.add (EL_INTERNAL_ID_AUDIO_ROUTER);
    results.add (EL_INTERNAL_ID_MIDI_ROUTER);
    results.add (EL_INTERNAL_ID_MIDI_PROGRAM_MAP);
    results.add (EL_INTERNAL_ID_MIDI_MONITOR);
    results.add (EL_INTERNAL_ID_OSC_RECEIVER);
    results.add (EL_INTERNAL_ID_OSC_SENDER);
   #if EL_USE_LUA
    results.add (EL_INTERNAL_ID_LUA);
    results.add (EL_INTERNAL_ID_SCRIPT);
   #endif
    results.add (EL_INTERNAL_ID_PLACEHOLDER);
   #endif // product enablements
    return results;
}

AudioPluginInstance* ElementAudioPluginFormat::instantiatePlugin (const PluginDescription& desc, 
                                                                    double sampleRate, int blockSize)
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

   #if defined (EL_PRO)
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_GRAPH)
        base = new SubGraphProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_AUDIO_MIXER)
        base = new AudioMixerProcessor (4, sampleRate, blockSize);
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_CHANNELIZE)
        base = new ChannelizeProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MIDI_CHANNEL_MAP)
        base = new MidiChannelMapProcessor();
   #endif // EL_PRO

   #if defined (EL_PRO) || defined (EL_SOLO)
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_AUDIO_FILE_PLAYER)
        base = new AudioFilePlayerNode();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MEDIA_PLAYER)
        base = new MediaPlayerProcessor();
    else if (desc.fileOrIdentifier == EL_INTERNAL_ID_PLACEHOLDER)
        base = new PlaceholderProcessor();
   #endif // EL_PRO || EL_SOLO

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

}
