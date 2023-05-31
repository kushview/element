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

#pragma once

#include "engine/nodes/BaseProcessor.h"
#include "engine/midichannelmap.hpp"

namespace element {

class MidiChannelMapProcessor : public BaseProcessor,
                                public AudioProcessorParameter::Listener
{
    Array<AudioParameterInt*> params;
    MidiChannelMap channels;

public:
    MidiChannelMapProcessor()
    {
        setPlayConfigDetails (0, 0, 44100.0, 512);
        for (int i = 0; i < 16; ++i)
        {
            String identifier = "channel-";
            identifier << String (i + 1);
            String name = "Channel ";
            name << String (i + 1);
            auto param = new AudioParameterInt (identifier, name, 1, 16, i + 1);
            addLegacyParameter (param);
            params.add (param);
            param->addListener (this);
        }
    }

    ~MidiChannelMapProcessor()
    {
        for (auto* param : params)
            param->removeListener (this);
        params.clear();
    }

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        jassert (isPositiveAndBelow (parameterIndex, params.size()));
        ScopedLock sl (getCallbackLock());
        channels.set (parameterIndex + 1, *params.getUnchecked (parameterIndex));
    }

    void parameterGestureChanged (int, bool) override {}

    inline const String getName() const override { return "MIDI Channel Map"; }

    inline void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = EL_INTERNAL_ID_MIDI_CHANNEL_MAP;
        desc.descriptiveName = "MIDI Channel Map";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_INTERNAL_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    inline AudioProcessorEditor* createEditor() override { return nullptr; }
    inline bool hasEditor() const override { return false; }

    inline void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        setPlayConfigDetails (0, 0, sampleRate, maximumExpectedSamplesPerBlock);
    }

    inline void releaseResources() override {}

    inline void processBlock (AudioBuffer<float>&, MidiBuffer& midiMessages) override
    {
        ScopedLock sl (getCallbackLock());
        channels.render (midiMessages);
    }

    inline double getTailLengthSeconds() const override { return 0; }
    inline bool acceptsMidi() const override { return true; }
    inline bool producesMidi() const override { return true; }
    inline bool supportsMPE() const override { return false; }
    inline bool isMidiEffect() const override { return true; }

    inline void getStateInformation (juce::MemoryBlock& destData) override
    {
        ValueTree state ("state");
        int chans[16] = { 0 };

        {
            ScopedLock sl (getCallbackLock());
            for (int ch = 0; ch < 16; ++ch)
                chans[ch] = *params.getUnchecked (ch);
        }

        for (int ch = 0; ch < 16; ++ch)
        {
            String key = "channel-";
            key << ch;
            state.setProperty (key, chans[ch], 0);
        }

        MemoryOutputStream stream (destData, false);
        state.writeToStream (stream);
    }

    inline void setStateInformation (const void* data, int sizeInBytes) override
    {
        const auto state = ValueTree::readFromData (data, (size_t) sizeInBytes);
        if (! state.hasType ("state"))
            return;

        int chans[16] = { 0 };
        for (int ch = 0; ch < 16; ++ch)
        {
            String key = "channel-";
            key << ch;
            chans[ch] = (int) state.getProperty (key, ch + 1);
        }

        {
            ScopedLock sl (getCallbackLock());
            for (int ch = 0; ch < 16; ++ch)
                *params.getUnchecked (ch) = chans[ch];
        }
    }

    inline int getNumPrograms() override { return 1; }
    inline int getCurrentProgram() override { return 0; }
    inline void setCurrentProgram (int index) override { ignoreUnused (index); }
    inline const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return "";
    }
    inline void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

protected:
    inline bool isBusesLayoutSupported (const BusesLayout&) const override { return false; }

#if 0
    // Audio Processor Template
    virtual StringArray getAlternateDisplayNames() const;
    

    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    
    virtual bool canAddBus (bool isInput) const                     { ignoreUnused (isInput); return false; }
    virtual bool canRemoveBus (bool isInput) const                  { ignoreUnused (isInput); return false; }
    virtual bool supportsDoublePrecisionProcessing() const;
    
    virtual void reset();
    virtual void setNonRealtime (bool isNonRealtime) noexcept;
    
    virtual void getCurrentProgramStateInformation (juce::MemoryBlock& destData);
    
    virtual void setCurrentProgramStateInformation (const void* data, int sizeInBytes);
    
    virtual void numChannelsChanged();
    virtual void numBusesChanged();
    virtual void processorLayoutsChanged();
    
    virtual void addListener (AudioProcessorListener* newListener);
    virtual void removeListener (AudioProcessorListener* listenerToRemove);
    virtual void setPlayHead (AudioPlayHead* newPlayHead);
    
    virtual void updateTrackProperties (const TrackProperties& properties);
    
protected:
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif
private:
    MidiBuffer tempMidi;
};

} // namespace element
