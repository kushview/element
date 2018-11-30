#pragma once

#include "engine/BaseProcessor.h"

namespace Element {

namespace Midi {

inline static int getChannel (const uint8 *data)
{
    if ((data[0] & 0xf0) != 0xf0)
        return (data[0] & 0xf) + 1;
    return 0;
}

inline static int setChannel (uint8 *data, const int channel)
{
    jassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16
    if ((data[0] & 0xf0) != (uint8) 0xf0)
        data[0] = (uint8) ((data[0] & (uint8) 0xf0)
                            | (uint8)(channel - 1));
}

}

class ChannelizeProcessor : public BaseProcessor
{
    AudioParameterInt* channel = nullptr;
public:
    
    ChannelizeProcessor()
    {
        addParameter (channel = new AudioParameterInt ("channel", "Out Channel", 0, 16, 0));
    }

    ~ChannelizeProcessor()
    {
        channel = nullptr;
    }

    const String getName() const override { return "MIDI Chanellize"; }
    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = "Channelize";
        desc.descriptiveName    = "MIDI Channelize";
        desc.numInputChannels   = 0;
        desc.numOutputChannels  = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }
    
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override { }
    void releaseResources() override { }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        const int outChan = *channel;
        if (outChan <= 0)
            return;
        
        MidiBuffer::Iterator iter (midiMessages);
        const uint8* data = nullptr; int bytes = 0, frame = 0;
        while (iter.getNextEvent (data, frame, bytes))
        {
            if (Midi::getChannel (data) > 0)
                Midi::setChannel (data, outChan);
        }

        midiMessages.swapWith (tempMidi);
        tempMidi.clear();
    }

    double getTailLengthSeconds() { return 0; }
    bool acceptsMidi() const { return true; }
    bool producesMidi() const { return true; }
    bool supportsMPE() const override   { return false; }
    bool isMidiEffect() const override  { return true; }

    void getStateInformation (juce::MemoryBlock& destData) override
    {

    }

    void setStateInformation (const void* data, int sizeInBytes) override {

    }

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
    
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual bool hasEditor() const = 0;
    
    virtual int getNumPrograms()        { return 1; };
    virtual int getCurrentProgram()     { return 1; };
    virtual void setCurrentProgram (int index) { ignoreUnused (index); };
    virtual const String getProgramName (int index) { ignoreUnused (index); }
    virtual void changeProgramName (int index, const String& newName) { ignoreUnused (index, newName); }
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
    virtual bool isBusesLayoutSupported (const BusesLayout&) const          { return true; }
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif
private:
    MidiBuffer tempMidi;
};

}
