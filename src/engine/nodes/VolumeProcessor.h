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

namespace Element {
    
class VolumeProcessor : public BaseProcessor
{
private:
    const bool stereo;
    float lastVolume;
    float gain;
    float lastGain;
    AudioParameterFloat* volume = nullptr;
    
public:
    explicit VolumeProcessor (const double minDb, const double maxDb,
                                const bool _stereo = false)
        : BaseProcessor(),
            stereo (_stereo)
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);
        addParameter (volume = new AudioParameterFloat (Tags::volume.toString(),
                                                        "Volume", minDb, maxDb, 0.f));
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
        desc.fileOrIdentifier   = stereo ? "element.volume.stereo" : "element.volume.mono";
        desc.descriptiveName    = stereo ? "Volume (stereo)" : "Volume (mono)";
        desc.numInputChannels   = stereo ? 2 : 1;
        desc.numOutputChannels  = stereo ? 2 : 1;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }
    
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                                sampleRate, maximumExpectedSamplesPerBlock);
    }
    
    void releaseResources() override
    {
        
    }
    
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (lastVolume != (float) *volume) {
            gain = (float)*volume <= -30.f ? 0.f : Decibels::decibelsToGain ((float) *volume);
        }
        
        for (int c = jmin (2, buffer.getNumChannels()); --c >= 0;)
            buffer.applyGainRamp (c, 0, buffer.getNumSamples(), lastGain, gain);
        
        lastGain = gain;
        lastVolume = *volume;
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
        state.setProperty (Tags::volume,  (float) *volume, 0);
        if (ScopedPointer<XmlElement> e = state.createXml())
            AudioProcessor::copyXmlToBinary (*e, destData);
    }
    
    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (ScopedPointer<XmlElement> e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
        {
            auto state = ValueTree::fromXml (*e);
            if (state.isValid())
            {
                *volume = lastVolume = (float) state.getProperty (Tags::volume,  (float) *volume);
                gain = lastGain = Decibels::decibelsToGain ((float) *volume);
            }
        }
    }
};

}
