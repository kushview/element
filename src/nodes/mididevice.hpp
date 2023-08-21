// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/signals.hpp>
#include "nodes/baseprocessor.hpp"

namespace element {

class MidiEngine;

class MidiDeviceProcessor : public BaseProcessor,
                            public MidiInputCallback,
                            private Timer
{
public:
    explicit MidiDeviceProcessor (const bool isInput, MidiEngine&);
    ~MidiDeviceProcessor() noexcept;

    boost::signals2::signal<void()> sigDeviceChanged;

    /** Returns the name of the Node.  MIDI in or out Device. */
    const String getName() const override
    {
        return inputDevice ? "MIDI In Device" : "MIDI Out Device";
    }

    /** Returns the name of the device currently loaded or trying to be loaded. */
    String getDeviceName() const noexcept;

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = "MIDI I/O Device";
        desc.fileOrIdentifier = inputDevice ? EL_NODE_ID_MIDI_INPUT_DEVICE
                                            : EL_NODE_ID_MIDI_OUTPUT_DEVICE;
        desc.uniqueId = inputDevice ? EL_NODE_UID_MIDI_INPUT_DEVICE
                                    : EL_NODE_UID_MIDI_OUTPUT_DEVICE;
        desc.descriptiveName = "MIDI device node";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = "Kushview, LLC";
        desc.pluginFormatName = "Internal";
        desc.version = "1.0.0";
    }

    double getLatency() const { return inputDevice ? 0.0 : midiOutLatency.get(); }
    void setLatency (double latencyMs);

    bool isInputDevice() const { return inputDevice; }
    bool isOutputDevice() const { return ! inputDevice; }

    Array<MidiDeviceInfo> getAvailableDevices() const noexcept;
    void setDevice (const MidiDeviceInfo& newDevice);
    Result closeDevice();

    /** Returns the device info that is currently running */
    MidiDeviceInfo getDevice() const noexcept { return device; }

    /** Returns the device info that is currently running */
    MidiDeviceInfo getWantedDevice() const noexcept { return deviceWanted; }

    bool isDeviceOpen() const;

    void reload();

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;
    void releaseResources() override;

    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return ! isInputDevice(); }
    bool producesMidi() const override { return isInputDevice(); }
    bool supportsMPE() const override { return false; }
    bool isMidiEffect() const override { return true; }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int index) override { ignoreUnused (index); }
    const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return String();
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void handleIncomingMidiMessage (MidiInput* source,
                                    const MidiMessage& message) override;

    void handlePartialSysexMessage (MidiInput* source,
                                    const uint8* messageData,
                                    int numBytesSoFar,
                                    double timestamp) override;

    inline bool canAddBus (bool isInput) const override
    {
        ignoreUnused (isInput);
        return false;
    }
    inline bool canRemoveBus (bool isInput) const override
    {
        ignoreUnused (isInput);
        return false;
    }

protected:
    inline bool isBusesLayoutSupported (const BusesLayout&) const override { return false; }
    inline bool canApplyBusesLayout (const BusesLayout& layouts) const override { return isBusesLayoutSupported (layouts); }
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
    MidiEngine& midi;
    bool prepared = false;
    MidiDeviceInfo device; // actual device name in use;
    MidiDeviceInfo deviceWanted; // The device as saved in Stage and chosen by users.
    MidiMessageCollector inputMessages;
    std::unique_ptr<MidiInput> input;
    std::unique_ptr<MidiOutput> output;
    Atomic<double> midiOutLatency { 0.0 };

    void waitForDevice() {}
    void timerCallback() override;
    bool deviceIsAvailable (const String& name);
    bool deviceIsAvailable (const MidiDeviceInfo& dev);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDeviceProcessor);
};

} // namespace element
