// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "engine/nodes/BaseProcessor.h"

namespace element {

class MediaPlayerProcessor : public BaseProcessor,
                             public AudioProcessorParameter::Listener
{
public:
    enum Parameters
    {
        Playing = 0,
        Slave,
        Volume
    };

    MediaPlayerProcessor();
    virtual ~MediaPlayerProcessor();

    void openFile (const File& file);
    const File& getAudioFile() const { return audioFile; }
    String getWildcard() const { return formats.getWildcardForAllFormats(); }

    void fillInPluginDescription (PluginDescription& desc) const override;

    const String getName() const override { return "Media Player"; }
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

    bool canAddBus (bool isInput) const override
    {
        juce::ignoreUnused (isInput);
        return false;
    }
    bool canRemoveBus (bool isInput) const override
    {
        juce::ignoreUnused (isInput);
        return false;
    }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool supportsMPE() const override { return false; }
    bool isMidiEffect() const override { return false; }

    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); };
    const String getProgramName (int index) override
    {
        juce::ignoreUnused (index);
        return getName();
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;

    AudioTransportSource& getPlayer() { return player; }

protected:
    bool isBusesLayoutSupported (const BusesLayout&) const override;

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

protected:
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif

private:
    TimeSliceThread thread { "MediaPlayer" };
    std::unique_ptr<AudioFormatReaderSource> reader;
    AudioFormatManager formats;
    AudioTransportSource player;

    AudioParameterBool* slave { nullptr };
    AudioParameterBool* playing { nullptr };
    AudioParameterFloat* volume { nullptr };

    File audioFile;

    void clearPlayer();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MediaPlayerProcessor)
};

} // namespace element
