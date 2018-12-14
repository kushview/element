/*
    InternalFormat.cpp - This file is part of Element
    Copyright (C) 2016-2018 Kushview, LLC.  All rights reserved.
*/

#include "ElementApp.h"

#include "engine/nodes/AllPassFilterNode.h"
#include "engine/nodes/MediaPlayerProcessor.h"
#include "engine/nodes/MidiChannelSplitterNode.h"

#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/InternalFormat.h"
#include "engine/AudioMixerProcessor.h"
#include "engine/ChannelizeProcessor.h"
#include "engine/CombFilterProcessor.h"
#include "engine/MidiDeviceProcessor.h"
#include "engine/MidiSequenceProcessor.h"
#include "engine/MidiChannelMapProcessor.h"
#include "engine/PlaceholderProcessor.h"
#include "engine/ReverbProcessor.h"
#include "engine/SubGraphProcessor.h"
#include "engine/VolumeProcessor.h"
#include "engine/WetDryProcessor.h"

#include "session/Session.h"
#include "session/UnlockStatus.h"

#include "Globals.h"

namespace Element {
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    
    InternalFormat::InternalFormat (AudioEngine& e)
        : engine (e)
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
        
        {
            PlaceholderProcessor p;
            p.fillInPluginDescription (placeholderDesc);
        }

        {
            MidiDeviceProcessor in (true);
            in.fillInPluginDescription (midiInputDeviceDesc);
            MidiDeviceProcessor out (false);
            out.fillInPluginDescription (midiOutputDeviceDesc);
        }
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
        else if (desc.fileOrIdentifier == "element.midiInputDevice")
        {
            return new MidiDeviceProcessor (true);
        }
        else if (desc.fileOrIdentifier == "element.midiOutputDevice")
        {
            return new MidiDeviceProcessor (false);
        }
        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_PLACEHOLDER)
        {
            return new PlaceholderProcessor();
        }

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
                                               int initialBufferSize, void* userData,
                                               void (*callback) (void*, AudioPluginInstance*, const String&))
    {
        if (auto* i = instantiatePlugin (d, initialSampleRate, initialBufferSize))
            callback (userData, i, String());
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
        else if (fileOrId == EL_INTERNAL_ID_GRAPH)
        {
            auto* const desc = ds.add (new PluginDescription());
            SubGraphProcessor().fillInPluginDescription (*desc);
        }
        else if (fileOrId == EL_INTERNAL_ID_MIDI_SEQUENCER)
        {
            auto* const desc = ds.add (new PluginDescription());
            MidiSequenceProcessor().fillInPluginDescription (*desc);
        }
        else if (fileOrId == EL_INTERNAL_ID_AUDIO_MIXER)
        {
            auto* const desc = ds.add (new PluginDescription());
            AudioMixerProcessor(4).fillInPluginDescription (*desc);
        }
        else if (fileOrId == EL_INTERNAL_ID_PLACEHOLDER)
        {
            auto* const desc = ds.add (new PluginDescription());
            PlaceholderProcessor().fillInPluginDescription (*desc);
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
        else if (fileOrId == EL_INTERNAL_ID_MEDIA_PLAYER)
        {
            auto* const desc = ds.add (new PluginDescription());
            MediaPlayerProcessor().fillInPluginDescription (*desc);
        }
    }
    
    StringArray ElementAudioPluginFormat::searchPathsForPlugins (const FileSearchPath&, bool /*recursive*/, bool /*allowAsync*/)
    {
        StringArray results;
        results.add ("element.comb");
        results.add ("element.allPass");
        results.add ("element.volume");
        results.add (EL_INTERNAL_ID_WET_DRY);
        results.add (EL_INTERNAL_ID_REVERB);
        results.add (EL_INTERNAL_ID_PLACEHOLDER);
        results.add (EL_INTERNAL_ID_CHANNELIZE);
        results.add (EL_INTERNAL_ID_MIDI_CHANNEL_MAP);
        results.add (EL_INTERNAL_ID_GRAPH);
        results.add (EL_INTERNAL_ID_AUDIO_MIXER);
        results.add (EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER);
        results.add (EL_INTERNAL_ID_MEDIA_PLAYER);

       #if EL_USE_MIDI_SEQUENCER
        results.add (EL_INTERNAL_ID_MIDI_SEQUENCER);
       #endif

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
        
        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_GRAPH)
            base = (world.getUnlockStatus().isFullVersion() ? new SubGraphProcessor() : nullptr);
        
        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MIDI_SEQUENCER)
            base = (world.getUnlockStatus().isFullVersion() ? new MidiSequenceProcessor() : nullptr);
        
        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_AUDIO_MIXER)
            base = (world.getUnlockStatus().isFullVersion() ? new AudioMixerProcessor (4, sampleRate, blockSize) : nullptr);

        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_PLACEHOLDER)
            base = (world.getUnlockStatus().isFullVersion() ? new PlaceholderProcessor() : nullptr);

        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_CHANNELIZE)
            base = (world.getUnlockStatus().isFullVersion() ? new ChannelizeProcessor() : nullptr);

        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MIDI_CHANNEL_MAP)
            base = (world.getUnlockStatus().isFullVersion() ? new MidiChannelMapProcessor() : nullptr);

        else if (desc.fileOrIdentifier == EL_INTERNAL_ID_MEDIA_PLAYER)
            base = (world.getUnlockStatus().isFullVersion() ? new MediaPlayerProcessor() : nullptr);

        return base != nullptr ? base.release() : nullptr;
    }
    
    void ElementAudioPluginFormat::createPluginInstance (const PluginDescription& d, double initialSampleRate,
                                                         int initialBufferSize, void* userData,
                                                         void (*callback) (void*, AudioPluginInstance*, const String&))
    {
        if (auto* i = instantiatePlugin (d, initialSampleRate, initialBufferSize))
            callback (userData, i, String());
        else
            callback (userData, nullptr, String());
    }
    
    bool ElementAudioPluginFormat::requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept
    {
        return false;
    }
}
