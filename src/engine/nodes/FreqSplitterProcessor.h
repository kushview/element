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
#include "ElementApp.h"
#include "EQFilterProcessor.h"

namespace Element {
    class FreqSplitterProcessor : public BaseProcessor
    {
    public:
        explicit FreqSplitterProcessor()
            : BaseProcessor (BusesProperties()
                .withInput ("Main", AudioChannelSet::canonicalChannelSet (2))
                .withOutput ("Main", AudioChannelSet::canonicalChannelSet (6))),
            numChannelsIn (2),
            numChannelsOut (6)
        {
            setPlayConfigDetails (numChannelsIn, numChannelsOut, 44100.0, 1024);

            NormalisableRange<float> freqRange (20.0f, 22000.0f);
            freqRange.setSkewForCentre (1000.0f);


            addParameter (lowFreq  = new AudioParameterFloat ("lowFreq",  "Low Frequency [Hz]",  freqRange, 500.0f));
            addParameter (highFreq = new AudioParameterFloat ("highFreq", "High Frequency [Hz]", freqRange, 2000.0f));
        }

        const String getName() const override { return "Frequency Band Splitter"; }

        void fillInPluginDescription (PluginDescription& desc) const override
        {
            desc.name = getName();
            desc.fileOrIdentifier   = EL_INTERNAL_ID_FREQ_SPLITTER;
            desc.descriptiveName    = "Frequency Band Splitter";
            desc.numInputChannels   = 2;
            desc.numOutputChannels  = 6;
            desc.hasSharedContainer = false;
            desc.isInstrument       = false;
            desc.manufacturerName   = "Element";
            desc.pluginFormatName   = "Element";
            desc.version            = "1.0.0";
            desc.uid                = EL_INTERNAL_UID_FREQ_SPLITTER;
        }

        void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
        {
            const float butterQ = 0.7071f; // maximally flat passband
            auto setupFilter = [butterQ, sampleRate] (EQFilter& filt, float freq, EQFilter::Shape shape)
            {
                filt.setFrequency (freq);
                filt.setQ (butterQ);
                filt.setGain (1.0f);
                filt.setShape (shape);
                filt.reset (sampleRate);
            };

            for (int ch = 0; ch < 2; ++ch)
            {
                setupFilter (lowLPF[ch],  *lowFreq,  EQFilter::Shape::LowPass);
                setupFilter (lowHPF[ch],  *lowFreq,  EQFilter::Shape::HighPass);
                setupFilter (highLPF[ch], *highFreq, EQFilter::Shape::LowPass);
                setupFilter (highHPF[ch], *highFreq, EQFilter::Shape::HighPass);
            }

            setPlayConfigDetails (numChannelsIn, numChannelsOut, sampleRate, maximumExpectedSamplesPerBlock);
        }

        void releaseResources() override
        {
        }

        void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
        {
            const auto totalNumInputChannels  = getTotalNumInputChannels();
            const auto totalNumOutputChannels = getTotalNumOutputChannels();
            const auto numSamples = buffer.getNumSamples();

            // copy input into extra output channels
            for (int ch = 2; ch < totalNumOutputChannels; ++ch)
                buffer.copyFrom (ch, 0, buffer.getReadPointer (ch % 2), numSamples);

            // update filter parameters
            for (int c = 0; c < 2; ++c)
            {
                lowLPF[c].setFrequency (*lowFreq);
                lowHPF[c].setFrequency (*lowFreq);
                highLPF[c].setFrequency (*highFreq);
                highHPF[c].setFrequency (*highFreq);
            }

            // Low freq band
            for (int ch = 0; ch < 2; ++ch)
            {
                lowLPF[ch].processBlock (buffer.getWritePointer (ch), numSamples);
            }

            // Mid freq band
            for (int ch = 2; ch < 4; ++ch)
            {
                lowHPF[ch-2].processBlock (buffer.getWritePointer (ch), numSamples);
                highLPF[ch-2].processBlock (buffer.getWritePointer (ch), numSamples);
            }

            // High freq band
            for (int ch = 4; ch < 6; ++ch)
            {
                highHPF[ch-4].processBlock (buffer.getWritePointer (ch), numSamples);
            }
        }

        AudioProcessorEditor* createEditor() override   { return new GenericAudioProcessorEditor (this); }
        bool hasEditor() const override                 { return true; }

        double getTailLengthSeconds() const override    { return 0.0; };
        bool acceptsMidi() const override               { return false; }
        bool producesMidi() const override              { return false; }

        int getNumPrograms() override                                      { return 1; };
        int getCurrentProgram() override                                   { return 1; };
        void setCurrentProgram (int index) override                        { ignoreUnused (index); };
        const String getProgramName (int index) override                   { ignoreUnused (index); return "Parameter"; }
        void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

        void getStateInformation (juce::MemoryBlock& destData) override
        {
            ValueTree state (Tags::state);
            state.setProperty ("lowFreq",  (float) *lowFreq,  0);
            state.setProperty ("highFreq", (float) *highFreq, 0);
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
                    *lowFreq  = (float) state.getProperty ("lowFreq",  (float) *lowFreq);
                    *highFreq = (float) state.getProperty ("highFreq", (float) *highFreq);
                }
            }
        }

        void numChannelsChanged() override
        {
            numChannelsIn = getTotalNumInputChannels();
            numChannelsOut = getTotalNumOutputChannels();
        }

    protected:
        inline bool isBusesLayoutSupported (const BusesLayout& layout) const override 
        {
            // supports single bus only
            if (layout.inputBuses.size() != 1 && layout.outputBuses.size() != 1)
                return false;

            // ins must equal outs
            // if (layout.getMainInputChannels() != layout.getMainOutputChannels())
            //     return false;

            const auto nchansIn = layout.getMainInputChannels();
            const auto nchansOut = layout.getMainOutputChannels();
            return nchansIn == 2 && nchansOut == 6;
        }

        inline bool canApplyBusesLayout (const BusesLayout& layouts) const override { return isBusesLayoutSupported (layouts); }
        inline bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties) override
        {
            ignoreUnused (isInput, isAddingBuses, outNewBusProperties);
            return false;
        }

    private:
        int numChannelsIn = 0;
        int numChannelsOut = 0;
        AudioParameterFloat* lowFreq    = nullptr;
        AudioParameterFloat* highFreq   = nullptr;
        EQFilter lowLPF[2];
        EQFilter lowHPF[2];
        EQFilter highLPF[2];
        EQFilter highHPF[2];
    };

}
