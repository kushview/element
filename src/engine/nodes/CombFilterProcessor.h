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

class CombFilter
{
public:
    CombFilter() noexcept
        : bufferSize (0), bufferIndex (0), allocatedSize (0), last (0) {}

    void setSize (const int numSamples)
    {
        if (numSamples == bufferSize)
            return;

        const int proposedSize = nextPowerOfTwo (numSamples);
        if (proposedSize > allocatedSize)
        {
            buffer.realloc (static_cast<size_t> (proposedSize));
            allocatedSize = proposedSize;
        }

        bufferSize = numSamples;
        if (bufferIndex >= bufferSize)
        {
            bufferIndex = 0;
        }

        clear();
    }

    void clear() noexcept
    {
        last = 0;
        bufferIndex = 0;
        buffer.clear ((size_t) bufferSize);
    }

    void free()
    {
        last = 0;
        bufferSize = allocatedSize = 0;
        buffer.free();
    }

    float process (const float input, const float damp, const float feedbackLevel) noexcept
    {
        const float output = buffer[bufferIndex];
        last = (output * (1.0f - damp)) + (last * damp);
        JUCE_UNDENORMALISE (last);

        float temp = input + (last * feedbackLevel);
        JUCE_UNDENORMALISE (temp);
        buffer[bufferIndex] = temp;
        bufferIndex = (bufferIndex + 1) % bufferSize;
        return output;
    }

private:
    HeapBlock<float> buffer;
    int bufferSize, bufferIndex, allocatedSize;
    float last;

    JUCE_DECLARE_NON_COPYABLE (CombFilter)
};

class CombFilterProcessor : public BaseProcessor
{
private:
    const bool stereo;
    AudioParameterFloat* length = nullptr;
    AudioParameterFloat* damping = nullptr;
    AudioParameterFloat* feedback = nullptr;

public:
    /*
         // Freeverb tunings
         25.306122448979592, 26.938775510204082,
         28.956916099773243, 30.748299319727891,
         32.244897959183673, 33.80952380952381,
         35.306122448979592, 36.666666666666667
     */
    explicit CombFilterProcessor (const bool _stereo = false)
        : BaseProcessor(),
          stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);
        addLegacyParameter (length = new AudioParameterFloat ("length", "Buffer Length", 1.f, 500.f, 90.f));
        lastLength = *length;
        addLegacyParameter (damping = new AudioParameterFloat ("damping", "Damping", 0.f, 1.f, 0.f));
        addLegacyParameter (feedback = new AudioParameterFloat ("feedback", "Feedback Level", 0.f, 1.f, 0.5f));
    }

    virtual ~CombFilterProcessor()
    {
        length = nullptr;
    }

    const String getName() const override { return "Comb Filter"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = stereo ? "element.comb.stereo" : "element.comb.mono";
        desc.descriptiveName = stereo ? "Comb Filter (stereo)" : "Comb Filter (mono)";
        desc.numInputChannels = stereo ? 2 : 1;
        desc.numOutputChannels = stereo ? 2 : 1;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_INTERNAL_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    int spreadForChannel (const int c) const
    {
        return c * 28;
    }
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        lastLength = *length;
        for (int i = 0; i < 2; ++i)
            comb[i].setSize (spreadForChannel (i) + (*length * sampleRate * 0.001));
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, sampleRate, maximumExpectedSamplesPerBlock);
    }

    void releaseResources() override
    {
        for (int i = 0; i < 2; ++i)
            comb[i].free();
    }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastLength != *length)
        {
            const int newSize = roundToIntAccurate (*length * getSampleRate() * 0.001);
            for (int i = 0; i < 2; ++i)
                comb[i].setSize (newSize);
            lastLength = *length;
        }

        const int numChans = jmin (2, buffer.getNumChannels());
        auto input = buffer.getArrayOfReadPointers();
        auto output = buffer.getArrayOfWritePointers();
        for (int c = 0; c < numChans; ++c)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                output[c][i] = comb[c].process (input[c][i], *damping, *feedback);
    }

    AudioProcessorEditor* createEditor() override { return new GenericAudioProcessorEditor (this); }
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
        state.setProperty ("damping", (float) *damping, 0);
        state.setProperty ("feedback", (float) *feedback, 0);
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
            {
                *damping = (float) state.getProperty ("damping", (float) *damping);
                *feedback = (float) state.getProperty ("feedback", (float) *feedback);
                *length = (float) state.getProperty ("length", (float) *length);
            }
        }
    }

private:
    CombFilter comb[2];
    float lastLength;
};

} // namespace element
