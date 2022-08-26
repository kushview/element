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

namespace element {

class WetDryProcessor : public BaseProcessor
{
private:
    AudioParameterFloat* wetLevel = nullptr;
    AudioParameterFloat* dryLevel = nullptr;
    float lastWetLevel = 0.33f;
    float lastDryLevel = 0.40f;

public:
    explicit WetDryProcessor()
        : BaseProcessor()
    {
        setPlayConfigDetails (4, 2, 44100.0, 1024);
        addLegacyParameter (wetLevel = new AudioParameterFloat ("wetLevel", "Wet Level", 0.f, 1.f, 0.33f));
        addLegacyParameter (dryLevel = new AudioParameterFloat ("dryLevel", "Dry Level", 0.f, 1.f, 0.40f));
    }

    virtual ~WetDryProcessor()
    {
        wetLevel = dryLevel = nullptr;
    }

    const String getName() const override { return "Wet/Dry"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = EL_INTERNAL_ID_WET_DRY;
        desc.version = "1.0.0";
        desc.descriptiveName = "Combines stereo wet/dry signals in to a single stereo output.";
        desc.numInputChannels = 4;
        desc.numOutputChannels = 2;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = "Element";
        desc.pluginFormatName = "Element";
    }

    void setLevels (const float newWet, const float newDry)
    {
        const float wetScaleFactor = 3.0f;
        const float dryScaleFactor = 2.0f;
        const float width = 1.f;
        const float wet = newWet * wetScaleFactor;

        dryGain.setValue (newDry * dryScaleFactor);
        wetGain1.setValue (0.5f * wet * (1.0f + width));
        wetGain2.setValue (0.5f * wet * (1.0f - width));
    }

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        setPlayConfigDetails (4, 2, sampleRate, maximumExpectedSamplesPerBlock);
        const double smoothTime = 0.01;
        dryGain.reset (sampleRate, smoothTime);
        wetGain1.reset (sampleRate, smoothTime);
        wetGain2.reset (sampleRate, smoothTime);
        lastWetLevel = (float) *wetLevel;
        lastDryLevel = (float) *dryLevel;
    }

    void releaseResources() override {}

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastWetLevel != (float) *wetLevel || lastDryLevel != (float) *dryLevel)
            setLevels (*wetLevel, *dryLevel);

        if (buffer.getNumChannels() >= 4)
        {
            const int numSamples = buffer.getNumSamples();
            const auto** input = buffer.getArrayOfReadPointers();
            auto** output = buffer.getArrayOfWritePointers();

            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = dryGain.getNextValue();
                const float wet1 = wetGain1.getNextValue();
                const float wet2 = wetGain2.getNextValue();

                output[0][i] = input[0][i] * wet1 + input[1][i] * wet2 + input[2][i] * dry;
                output[1][i] = input[1][i] * wet1 + input[0][i] * wet2 + input[3][i] * dry;
            }
        }
        else
        {
            DBG ("CHans: " << buffer.getNumChannels());
        }

        lastWetLevel = *wetLevel;
        lastDryLevel = *dryLevel;
    }

    AudioProcessorEditor* createEditor() override
    {
        auto* ed = new GenericAudioProcessorEditor (this);
        ed->resized();
        return ed;
    }

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
        ValueTree state (Tags::state);
        state.setProperty ("wetLevel", (float) *wetLevel, 0);
        state.setProperty ("dryLevel", (float) *dryLevel, 0);
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
                *wetLevel = (float) state.getProperty ("wetLevel", (float) *wetLevel);
                *dryLevel = (float) state.getProperty ("dryLevel", (float) *dryLevel);
            }
        }
    }

private:
    LinearSmoothedValue<float> dryGain, wetGain1, wetGain2;
};

} // namespace element
