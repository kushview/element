
#pragma once

#include "engine/BaseProcessor.h"
#include "session/Node.h"

namespace Element {

class PlaceholderProcessor : public BaseProcessor
{    
public:
    PlaceholderProcessor() { }
    PlaceholderProcessor (const Node& node, double sampleRate = 44100.0, 
                          int blockSize = 1024)
    { 
        setupFor (node, sampleRate, blockSize);
    }
    
    PlaceholderProcessor (const int numAudioIns, const int numAudioOuts,
                          const bool hasMidiIn, const bool hasMidiOut)
        : numInputs (numAudioIns), numOutputs (numAudioOuts),
          acceptMidi (hasMidiIn), produceMidi (hasMidiOut)
    {
        setPlayConfigDetails (numInputs, numOutputs, 44100.0, 1024);
    }

    virtual ~PlaceholderProcessor() { }

    inline const String getName() const override { return "Placeholder"; };
    
    inline void setupFor (const Node& node, double sampleRate, int bufferSize)
    {
        PortArray ins, outs;
        node.getPorts (ins, outs, PortType::Audio);
        numInputs       = ins.size();
        numOutputs      = outs.size();
        
        ins.clearQuick(); outs.clearQuick();
        node.getPorts (ins, outs, PortType::Midi);
        acceptMidi      = ins.size() > 0;
        produceMidi     = outs.size() > 0;
        
        for (int i = 0; i < node.getPortsValueTree().getNumChildren(); ++i)
        {
            const auto port = node.getPort (i);
            if (port.isA (PortType::Control, true))
                addParameter (new AudioParameterFloat (port.getName(), port.getName(), 0, 1, 0));
        }

        setPlayConfigDetails (numInputs, numOutputs, sampleRate, bufferSize);
    }
    
    inline void fillInPluginDescription (PluginDescription& d) const override
    {
        d.name = "Placeholder";
        d.version = "1.0.0";
        d.pluginFormatName = "Element";
        d.manufacturerName = "Element";
        d.fileOrIdentifier = "element.placeholder";
        d.numInputChannels = numInputs;
        d.numOutputChannels = numOutputs;
    }
    
    inline void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override {
        setPlayConfigDetails (numInputs, numOutputs, sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    inline void releaseResources() override { }
    inline void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override {
        processBlockBypassed (buffer, midiMessages);
    }
    
    inline double getTailLengthSeconds() const override { return 0.0; }
    
    inline bool acceptsMidi() const override       { return acceptMidi; }
    inline bool producesMidi() const override      { return produceMidi; }
    
    inline AudioProcessorEditor* createEditor() override { return nullptr; }
    inline bool hasEditor() const override { return false; }
    
    inline void getStateInformation (juce::MemoryBlock&) override { }
    inline void setStateInformation (const void*, int) override { }

    inline int getNumPrograms() override        { return 1; };
    inline int getCurrentProgram() override     { return 1; };
    inline void setCurrentProgram (int index) override { ignoreUnused (index); };
    inline const String getProgramName (int index) override { ignoreUnused (index); return ""; }
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
    int numInputs       = 2;
    int numOutputs      = 2;
    bool acceptMidi     = true;
    bool produceMidi    = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceholderProcessor)
};

}
