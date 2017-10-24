
#pragma once

#include "ElementApp.h"

namespace Element {

class BaseProcessor : public AudioPluginInstance
{
public:
    BaseProcessor() { }
    virtual ~BaseProcessor() { }
    
#if 0
    // Audio Processor Template
    virtual const String getName() const = 0;
    virtual StringArray getAlternateDisplayNames() const;
    virtual void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    
    virtual void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) = 0;
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);

    virtual bool canAddBus (bool isInput) const                     { ignoreUnused (isInput); return false; }
    virtual bool canRemoveBus (bool isInput) const                  { ignoreUnused (isInput); return false; }
    virtual bool supportsDoublePrecisionProcessing() const;
    
    virtual double getTailLengthSeconds() const = 0;
    virtual bool acceptsMidi() const = 0;
    virtual bool producesMidi() const = 0;
    virtual bool supportsMPE() const                            { return false; }
    virtual bool isMidiEffect() const                           { return false; }
    virtual void reset();
    virtual void setNonRealtime (bool isNonRealtime) noexcept;
    
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual bool hasEditor() const = 0;
    
    virtual int getNumPrograms()        { return 1; };
    virtual int getCurrentProgram()     { return 1; };
    virtual void setCurrentProgram (int index) { ignoreUnused (index); };
    virtual const String getProgramName (int index) { ignoreUnused (index); }
    virtual void changeProgramName (int index, const String& newName) { ignoreUnused (index, newName); }
    virtual void getStateInformation (juce::MemoryBlock& destData) = 0;
    virtual void getCurrentProgramStateInformation (juce::MemoryBlock& destData);
   
    virtual void setStateInformation (const void* data, int sizeInBytes) = 0;
    virtual void setCurrentProgramStateInformation (const void* data, int sizeInBytes);
    
    virtual void numChannelsChanged();
    virtual void numBusesChanged();
    virtual void processorLayoutsChanged();
    
    virtual void addListener (AudioProcessorListener* newListener);
    virtual void removeListener (AudioProcessorListener* listenerToRemove);
    virtual void setPlayHead (AudioPlayHead* newPlayHead);
    
    virtual void updateTrackProperties (const TrackProperties& properties);
    
    virtual void fillInPluginDescription (PluginDescription& description);
    
protected:
    virtual bool isBusesLayoutSupported (const BusesLayout&) const          { return true; }
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};

class CombFilterProcessor : public BaseProcessor
{
private:
    const bool stereo;
    
public:
    explicit CombFilterProcessor (const bool _stereo = false)
        : BaseProcessor(),
          stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                              44100.0, 1024);
    }
    
    virtual ~CombFilterProcessor() { }
    
    const String getName() const override { return "Comb Filter"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = stereo ? "element.combFilter.stereo" : "element.combFilter.mono";
        desc.descriptiveName    = stereo ? "Comb Filter (stereo)" : "Comb Filter (mono)";
        desc.numInputChannels   = stereo ? 2 : 1;
        desc.numOutputChannels  = stereo ? 2 : 1;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }
    
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                              sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    void releaseResources() override
    {
        
    }
    
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        buffer.clear (0, buffer.getNumSamples());
    }

    AudioProcessorEditor* createEditor() override   { return nullptr; }
    bool hasEditor() const override                 { return false; }
    
    double getTailLengthSeconds() const override    { return 0.0; };
    bool acceptsMidi() const override               { return false; }
    bool producesMidi() const override              { return false; }
    
    int getNumPrograms() override                                      { return 1; };
    int getCurrentProgram() override                                   { return 1; };
    void setCurrentProgram (int index) override                        { ignoreUnused (index); };
    const String getProgramName (int index) override                   { ignoreUnused (index); return "Parameter"; }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }
    void getStateInformation (juce::MemoryBlock& destData) override    { ignoreUnused (destData); }
    void setStateInformation (const void* data, int sizeInBytes) override { ignoreUnused (data, sizeInBytes); }
};

}
