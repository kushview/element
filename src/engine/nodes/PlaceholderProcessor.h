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
#include "session/node.hpp"

namespace Element {

class PlaceholderProcessor : public BaseProcessor
{
public:
    PlaceholderProcessor()
        : BaseProcessor (BusesProperties()
                             .withInput ("Main", AudioChannelSet::stereo())
                             .withOutput ("Main", AudioChannelSet::stereo()))
    {
        numInputs = numOutputs = 2;
        acceptMidi = produceMidi = true;
    }

    PlaceholderProcessor (const Node& node, double sampleRate = 44100.0, int blockSize = 1024)
    {
        setupFor (node, sampleRate, blockSize);
    }

    PlaceholderProcessor (const int numAudioIns, const int numAudioOuts, const bool hasMidiIn, const bool hasMidiOut)
        : BaseProcessor (BusesProperties()
                             .withInput ("Main", AudioChannelSet::namedChannelSet (numAudioIns))
                             .withOutput ("Main", AudioChannelSet::namedChannelSet (numAudioOuts))),
          numInputs (numAudioIns),
          numOutputs (numAudioOuts),
          acceptMidi (hasMidiIn),
          produceMidi (hasMidiOut)
    {
    }

    virtual ~PlaceholderProcessor() {}

    inline const String getName() const override { return "Placeholder"; };

    inline void setupFor (const Node& node, double sampleRate, int bufferSize)
    {
        PortArray ins, outs;
        node.getPorts (ins, outs, PortType::Audio);
        numInputs = ins.size();
        numOutputs = outs.size();
        setChannelLayoutOfBus (true, 0, AudioChannelSet::namedChannelSet (numInputs));
        setChannelLayoutOfBus (false, 0, AudioChannelSet::namedChannelSet (numOutputs));

        ins.clearQuick();
        outs.clearQuick();
        node.getPorts (ins, outs, PortType::Midi);
        acceptMidi = ins.size() > 0;
        produceMidi = outs.size() > 0;

        int controlIdx = 0;
        for (int i = 0; i < node.getPortsValueTree().getNumChildren(); ++i)
        {
            const auto port = node.getPort (i);
            if (port.isA (PortType::Control, true))
            {
                String controlId = "control-";
                controlId << controlIdx++;
                addParameter (new AudioParameterFloat (controlId, port.getName(), 0, 1, 0));
            }
        }
    }

    inline void fillInPluginDescription (PluginDescription& d) const override
    {
        d.name = "Placeholder";
        d.version = "1.0.0";
        d.pluginFormatName = "Element";
        d.manufacturerName = "Element";
        d.fileOrIdentifier = EL_INTERNAL_ID_PLACEHOLDER;
        d.numInputChannels = numInputs;
        d.numOutputChannels = numOutputs;
    }

    inline void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        setPlayConfigDetails (numInputs, numOutputs, sampleRate, maximumExpectedSamplesPerBlock);
    }

    inline void releaseResources() override {}
    inline void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        processBlockBypassed (buffer, midiMessages);
    }

    inline double getTailLengthSeconds() const override { return 0.0; }

    inline bool acceptsMidi() const override { return acceptMidi; }
    inline bool producesMidi() const override { return produceMidi; }

    inline AudioProcessorEditor* createEditor() override { return nullptr; }
    inline bool hasEditor() const override { return false; }

    inline void getStateInformation (juce::MemoryBlock&) override {}
    inline void setStateInformation (const void*, int) override {}

    inline int getNumPrograms() override { return 1; };
    inline int getCurrentProgram() override { return 1; };
    inline void setCurrentProgram (int index) override { ignoreUnused (index); };
    inline const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return "";
    }
    inline void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

#if 0
    // Audio Processor Template
    virtual StringArray getAlternateDisplayNames() const;
    
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    virtual bool canAddBus (bool isInput) const                     { ignoreUnused (isInput); return false; }
    virtual bool canRemoveBus (bool isInput) const                  { ignoreUnused (isInput); return false; }
    virtual bool supportsDoublePrecisionProcessing() const;
    virtual bool supportsMPE() const                            { return false; }
    virtual bool isMidiEffect() const                           { return false; }
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
    virtual void fillInPluginDescription (PluginDescription& description);

protected:
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);

#endif

protected:
    bool isBusesLayoutSupported (const BusesLayout& layout) const override
    {
        if (layout.inputBuses.size() > 1 || layout.outputBuses.size() > 1)
            return false;
        return layout.getNumChannels (true, 0) == numInputs && layout.getNumChannels (false, 0) == numOutputs;
    }

    bool canApplyBusesLayout (const BusesLayout& layouts) const override { return isBusesLayoutSupported (layouts); }

private:
    int numInputs = 2;
    int numOutputs = 2;
    bool acceptMidi = true;
    bool produceMidi = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaceholderProcessor)
};

} // namespace Element
