#pragma once

#include "engine/nodes/BaseProcessor.h"

namespace Element {

class MidiDeviceProcessor : public BaseProcessor,
                            public MidiInputCallback
{
public:
    explicit MidiDeviceProcessor (const bool isInput = true);
    ~MidiDeviceProcessor() noexcept;

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = "MIDI I/O Device";
        desc.fileOrIdentifier   = inputDevice ? "element.midiInputDevice" : "element.midiOutputDevice";
        desc.descriptiveName    = "MIDI device node";
        desc.numInputChannels   = 0;
        desc.numOutputChannels  = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Kushview, LLC";
        desc.pluginFormatName   = "Internal";
        desc.version            = "1.0.0";
    }

    bool isInputDevice()        const { return inputDevice; }
    bool isOutputDevice()       const { return !isInputDevice(); }

    void setCurrentDevice (const String& device);
    const String& getCurrentDevice() const { return deviceName; }
    bool isDeviceOpen() const;

    void reload();
    
    const String getName() const override;
    
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    void releaseResources() override;
    
    double getTailLengthSeconds()   const override { return 0; }
    bool acceptsMidi()              const override { return !isInputDevice(); }
    bool producesMidi()             const override { return isInputDevice(); }
    bool supportsMPE()              const override { return false; }
    bool isMidiEffect()             const override { return true; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int getNumPrograms()        override { return 1; }
    int getCurrentProgram()     override { return 1; }
    void setCurrentProgram (int index)          override { ignoreUnused (index); }
    const String getProgramName (int index)     override { ignoreUnused (index); return String(); }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }


    void handleIncomingMidiMessage (MidiInput* source,
                                    const MidiMessage& message) override;

    void handlePartialSysexMessage (MidiInput* source,
                                    const uint8* messageData,
                                    int numBytesSoFar,
                                    double timestamp) override;

    inline bool canAddBus (bool isInput) const override { ignoreUnused (isInput); return false; }
    inline bool canRemoveBus (bool isInput) const override { ignoreUnused (isInput); return false; }

protected:
    inline bool isBusesLayoutSupported (const BusesLayout&) const override         { return false; }
    inline bool canApplyBusesLayout (const BusesLayout& layouts) const override    { return isBusesLayoutSupported (layouts); }
    inline bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties) override 
    {
        ignoreUnused (isInput, isAddingBuses, outNewBusProperties);
        return false;
    }

#if 0
    // Audio Processor Template
    virtual StringArray getAlternateDisplayNames() const;
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    
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
#endif
    
private:
    const bool inputDevice;
    bool prepared = false;
    String deviceName;
    ScopedPointer<MidiInput> input;
    ScopedPointer<MidiOutput> output;
    MidiMessageCollector inputMessages;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDeviceProcessor);
};

}
