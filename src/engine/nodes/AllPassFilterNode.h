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

class AllPassFilter
{
public:
    AllPassFilter() noexcept : bufferSize (0), bufferIndex (0) {}

    void setSize (const int size)
    {
        if (size != bufferSize)
        {
            bufferIndex = 0;
            buffer.malloc ((size_t) size);
            bufferSize = size;
        }

        clear();
    }

    void clear() noexcept
    {
        bufferIndex = 0;
        buffer.clear ((size_t) bufferSize);
    }

    void free()
    {
        bufferSize = 0;
        bufferIndex = 0;
        buffer.free();
    }

    float process (const float input) noexcept
    {
        const float bufferedValue = buffer[bufferIndex];
        float temp = input + (bufferedValue * 0.5f);
        JUCE_UNDENORMALISE (temp);
        buffer[bufferIndex] = temp;
        bufferIndex = (bufferIndex + 1) % bufferSize;
        return bufferedValue - input;
    }

private:
    HeapBlock<float> buffer;
    int bufferSize, bufferIndex;

    JUCE_DECLARE_NON_COPYABLE (AllPassFilter)
};

class AllPassFilterProcessor : public BaseProcessor
{
private:
    const bool stereo;
    AudioParameterFloat* length = nullptr;

public:
    explicit AllPassFilterProcessor (const bool _stereo = false)
        : BaseProcessor(), stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);
        addLegacyParameter (length = new AudioParameterFloat ("length", "Buffer Length", 1.f, 500.f, 90.f));
        lastLength = *length;
    }

    virtual ~AllPassFilterProcessor()
    {
        length = nullptr;
    }

    const String getName() const override { return "AllPass Filter"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = stereo ? "element.allPass.stereo" : "element.allPass.mono";
        desc.descriptiveName = stereo ? "AllPass Filter (stereo)" : "AllPass Filter (mono)";
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
        lastLength = *length;
        for (int i = 0; i < 2; ++i)
            allPass[i].setSize (*length * sampleRate * 0.001);
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, sampleRate, maximumExpectedSamplesPerBlock);
    }

    void releaseResources() override
    {
        for (int i = 0; i < 2; ++i)
            allPass[i].free();
    }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastLength != *length)
        {
            const int newSize = roundToIntAccurate (*length * getSampleRate() * 0.001);
            for (int i = 0; i < 2; ++i)
                allPass[i].setSize (newSize);
            lastLength = *length;
        }

        const int numChans = jmin (2, buffer.getNumChannels());
        auto input = buffer.getArrayOfReadPointers();
        auto output = buffer.getArrayOfWritePointers();
        for (int c = 0; c < numChans; ++c)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                output[c][i] = allPass[c].process (input[c][i]);
    }

    AudioProcessorEditor* createEditor() override { return new GenericAudioProcessorEditor (this); }
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; };
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram (int index) override { ignoreUnused (index); };
    const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return getName();
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        ValueTree state (tags::state);
        state.setProperty ("length", (float) *length, 0);
        if (auto e = state.createXml())
            AudioProcessor::copyXmlToBinary (*e, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (auto e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
        {
            auto state = ValueTree::fromXml (*e);
            if (state.isValid())
                *length = (float) state.getProperty ("length", (float) *length);
        }
    }

private:
    AllPassFilter allPass[2];
    float lastLength;
};

} // namespace element
