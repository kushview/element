// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nodes/baseprocessor.hpp"

namespace element {

class VolumeProcessor : public BaseProcessor
{
private:
    const bool stereo;
    float lastVolume;
    float gain;
    float lastGain;
    AudioParameterFloat* volume = nullptr;

public:
    explicit VolumeProcessor (const double minDb, const double maxDb, const bool _stereo = false)
        : BaseProcessor (BusesProperties()
                             .withInput ("Main", _stereo ? AudioChannelSet::stereo() : AudioChannelSet::mono(), true)
                             .withOutput ("Main", _stereo ? AudioChannelSet::stereo() : AudioChannelSet::mono(), true)),
          stereo (_stereo)
    {
        addLegacyParameter (volume = new AudioParameterFloat (juce::ParameterID (tags::volume.toString(), 1),
                                                              "Volume",
                                                              minDb,
                                                              maxDb,
                                                              0.f));
        lastVolume = *volume;
        gain = Decibels::decibelsToGain (lastVolume);
        lastGain = gain;
    }

    virtual ~VolumeProcessor()
    {
        volume = nullptr;
    }

    const String getName() const override { return "Volume"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = stereo ? "element.volume.stereo" : "element.volume.mono";
        desc.descriptiveName = stereo ? "Volume (stereo)" : "Volume (mono)";
        desc.numInputChannels = stereo ? 2 : 1;
        desc.numOutputChannels = stereo ? 2 : 1;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, sampleRate, maximumExpectedSamplesPerBlock);
    }

    void releaseResources() override
    {
    }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastVolume != (float) *volume)
        {
            gain = (float) *volume <= -30.f ? 0.f : Decibels::decibelsToGain ((float) *volume);
        }

        for (int c = jmin (2, buffer.getNumChannels()); --c >= 0;)
            buffer.applyGainRamp (c, 0, buffer.getNumSamples(), lastGain, gain);

        lastGain = gain;
        lastVolume = *volume;
    }

    AudioProcessorEditor* createEditor() override { return new GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; };
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 1; };
    void setCurrentProgram (int index) override { ignoreUnused (index); };
    const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return "Parameter";
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        ValueTree state (tags::state);
        state.setProperty (tags::volume, (float) *volume, 0);
        if (auto e = state.createXml())
            AudioProcessor::copyXmlToBinary (*e, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (auto e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
        {
            auto state = ValueTree::fromXml (*e);
            if (state.isValid())
            {
                *volume = lastVolume = (float) state.getProperty (tags::volume, (float) *volume);
                gain = lastGain = Decibels::decibelsToGain ((float) *volume);
            }
        }
    }

protected:
    bool isBusesLayoutSupported (const BusesLayout& layout) const override
    {
        if (layout.inputBuses.size() != 1 || layout.outputBuses.size() != 1)
            return false;
        const int nchans = stereo ? 2 : 1;
        return layout.getMainInputChannels() == nchans
               && layout.getMainOutputChannels() == nchans;
    }

    bool canApplyBusCountChange (bool, bool, BusProperties&) override { return false; }
};

} // namespace element
