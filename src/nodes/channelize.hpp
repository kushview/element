// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "nodes/baseprocessor.hpp"

namespace element {

namespace Midi {

inline static int getChannel (const uint8* data)
{
    if ((data[0] & 0xf0) != 0xf0)
        return (data[0] & 0xf) + 1;
    return 0;
}

inline static void setChannel (uint8* data, const int channel)
{
    jassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16
    if ((data[0] & 0xf0) != (uint8) 0xf0)
        data[0] = (uint8) ((data[0] & (uint8) 0xf0)
                           | (uint8) (channel - 1));
}

} // namespace Midi

class ChannelizeProcessor : public BaseProcessor
{
    AudioParameterInt* channel = nullptr;

public:
    ChannelizeProcessor()
    {
        setPlayConfigDetails (0, 0, 44100.0, 512);
        addLegacyParameter (channel = new AudioParameterInt ("channel", "Out Channel", 1, 16, 1));
    }

    ~ChannelizeProcessor()
    {
        channel = nullptr;
    }

    inline const String getName() const override { return "MIDI Channelize"; }
    inline void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = EL_NODE_ID_CHANNELIZE;
        desc.descriptiveName = "MIDI Channelize";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
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

    inline void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        const int outChan = *channel;
        if (outChan <= 0)
            return;

        for (auto m : midiMessages)
        {
            auto msg = m.getMessage();
            if (msg.getChannel() > 0)
                msg.setChannel (outChan);
            tempMidi.addEvent (msg, m.samplePosition);
        }

        midiMessages.swapWith (tempMidi);
        tempMidi.clear();
    }

    inline double getTailLengthSeconds() const override { return 0; }
    inline bool acceptsMidi() const override { return true; }
    inline bool producesMidi() const override { return true; }
    inline bool supportsMPE() const override { return false; }
    inline bool isMidiEffect() const override { return true; }

    inline void getStateInformation (juce::MemoryBlock& destData) override
    {
        int chtoWrite = 1;
        {
            ScopedLock sl (getCallbackLock());
            chtoWrite = *channel;
        }

        destData.append (&chtoWrite, sizeof (int));
    }

    inline void setStateInformation (const void* data, int sizeInBytes) override
    {
        MemoryBlock block (data, sizeInBytes);
        ScopedLock sl (getCallbackLock());
        *channel = *reinterpret_cast<int*> (block.getData());
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

protected:
    inline bool isBusesLayoutSupported (const BusesLayout&) const override { return false; }

private:
    MidiBuffer tempMidi;
};

} // namespace element
