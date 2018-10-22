
#pragma once

#include "engine/BaseProcessor.h"
#include "session/Node.h"

namespace Element {

class PlaceholderProcessor : public BaseProcessor
{
    int numInputs  = 0;
    int numOutputs = 0;
    bool acceptMidi  = false;
    bool produceMidi = false;
    
public:
    PlaceholderProcessor() { }
    PlaceholderProcessor (const Node& node) { setupFor (node, 44100.0, 1024); }
    virtual ~PlaceholderProcessor() { }
    const String getName() const override { return "Placeholder"; };
    
    void setupFor (const Node& node, double sampleRate, int bufferSize)
    {
        PortArray ins, outs;
        node.getPorts (ins, outs, PortType::Audio);
        numInputs       = ins.size();
        numOutputs      = outs.size();
        
        ins.clearQuick(); outs.clearQuick();
        node.getPorts (ins, outs, PortType::Midi);
        acceptMidi      = ins.size() > 0;
        produceMidi     = outs.size() > 0;
        
        setPlayConfigDetails (numInputs, numOutputs, sampleRate, bufferSize);
    }
    
    void fillInPluginDescription (PluginDescription& d) const override
    {
        d.name = "Placeholder";
        d.version = "1.0.0";
        d.pluginFormatName = "Internal";
        d.fileOrIdentifier = "elemment.placeholder";
        d.numInputChannels = numInputs;
        d.numOutputChannels = numOutputs;
    }
    
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override {
        setPlayConfigDetails (numInputs, numOutputs, sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    void releaseResources() override { }
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override {
        processBlockBypassed (buffer, midiMessages);
    }
    
    double getTailLengthSeconds() const override { return 0.0; }
    
    bool acceptsMidi() const override       { return acceptMidi; }
    bool producesMidi() const override      { return produceMidi; }
    
    AudioProcessorEditor* createEditor() override { return 0; }
    bool hasEditor() const override { return false; }
    
    void getStateInformation (juce::MemoryBlock&) override { }
    void setStateInformation (const void*, int) override { }

    int getNumPrograms() override        { return 1; };
    int getCurrentProgram() override     { return 1; };
    void setCurrentProgram (int index) override { ignoreUnused (index); };
    const String getProgramName (int index) override { ignoreUnused (index); return ""; }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }
    
   #if 0
    // Audio Processor Template
    virtual StringArray getAlternateDisplayNames() const;
    
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    virtual bool canAddBus (bool isInput) const                     { ignoreUnused (isInput); return false; }
    virtual bool canRemoveBus (bool isInput) const                  { ignoreUnused (isInput); return false; }
    virtual bool supportsDoublePrecisionProcessing() const;
    virtual bool supportsMPE() const                            { return false; }
    virtual bool isMidiEffect() const                           { return false; }
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
    virtual void fillInPluginDescription (PluginDescription& description);
    
protected:
    virtual bool isBusesLayoutSupported (const BusesLayout&) const          { return true; }
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
   #endif
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceholderProcessor)
};

}
