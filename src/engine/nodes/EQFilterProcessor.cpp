/*
Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)

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

#include "EQFilterProcessor.h"
#include "gui/nodes/EQFilterNodeEditor.h"

namespace element {

EQFilterProcessor::EQFilterProcessor (const int _numChannels)
    : BaseProcessor (BusesProperties()
                         .withInput ("Main", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))
                         .withOutput ("Main", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))),
      numChannels (jlimit (1, 2, _numChannels))
{
    setPlayConfigDetails (numChannels, numChannels, 44100.0, 1024);

    NormalisableRange<float> freqRange (20.0f, 22000.0f);
    freqRange.setSkewForCentre (1000.0f);

    NormalisableRange<float> qRange (0.1f, 18.0f);
    qRange.setSkewForCentre (0.707f);

    addParameter (freq = new AudioParameterFloat ("freq", "Cutoff Frequency [Hz]", freqRange, 1000.0f));
    addParameter (q = new AudioParameterFloat ("q", "Filter Q", qRange, 0.707f));
    addParameter (gainDB = new AudioParameterFloat ("gain", "Filter Gain [dB]", -15.0f, 15.0f, 0.0f));
    addParameter (eqShape = new AudioParameterChoice ("shape", "EQ Shape", { "Bell", "Notch", "Hi Shelf", "Low Shelf", "HPF", "LPF" }, 0));
}

void EQFilterProcessor::fillInPluginDescription (PluginDescription& desc) const
{
    desc.name = getName();
    desc.fileOrIdentifier = EL_INTERNAL_ID_EQ_FILTER;
    desc.descriptiveName = "EQ Filter";
    desc.numInputChannels = 2;
    desc.numOutputChannels = 2;
    desc.hasSharedContainer = false;
    desc.isInstrument = false;
    desc.manufacturerName = "Element";
    desc.pluginFormatName = "Element";
    desc.version = "1.0.0";
    desc.uniqueId = EL_INTERNAL_UID_EQ_FILTER;
}

void EQFilterProcessor::updateParams()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        eqFilter[ch].setFrequency (*freq);
        eqFilter[ch].setQ (*q);
        eqFilter[ch].setGain (Decibels::decibelsToGain ((float) *gainDB));
        eqFilter[ch].setShape ((EQFilter::Shape) eqShape->getIndex());
    }
}

void EQFilterProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    updateParams();

    for (int ch = 0; ch < 2; ++ch)
        eqFilter[ch].reset (sampleRate);

    setPlayConfigDetails (numChannels, numChannels, sampleRate, maximumExpectedSamplesPerBlock);
}

void EQFilterProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    const int numChans = jmin (2, buffer.getNumChannels());
    auto** output = buffer.getArrayOfWritePointers();

    updateParams();

    for (int c = 0; c < numChans; ++c)
        eqFilter[c].processBlock (output[c], buffer.getNumSamples());
}

AudioProcessorEditor* EQFilterProcessor::createEditor()
{
    return new EQFilterNodeEditor (*this);
}

void EQFilterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state (Tags::state);
    state.setProperty ("freq", (float) *freq, 0);
    state.setProperty ("q", (float) *q, 0);
    state.setProperty ("gainDB", (float) *gainDB, 0);
    state.setProperty ("shape", (int) eqShape->getIndex(), 0);
    if (auto e = state.createXml())
        AudioProcessor::copyXmlToBinary (*e, destData);
}

void EQFilterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
    {
        auto state = ValueTree::fromXml (*e);
        if (state.isValid())
        {
            *freq = (float) state.getProperty ("freq", (float) *freq);
            *q = (float) state.getProperty ("q", (float) *q);
            *gainDB = (float) state.getProperty ("gainDB", (float) *gainDB);
            *eqShape = (int) state.getProperty ("shape", (int) *eqShape);
        }
    }
}

void EQFilterProcessor::numChannelsChanged()
{
    numChannels = getTotalNumInputChannels();
}

} // namespace element
