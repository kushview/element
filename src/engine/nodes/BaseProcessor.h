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

#include "JuceHeader.h"
#include "engine/nodes/NodeTypes.h"

namespace element {

class BaseProcessor : public AudioPluginInstance
{
public:
    BaseProcessor() : AudioPluginInstance() {}
    BaseProcessor (const BusesProperties& ioLayouts)
        : AudioPluginInstance (ioLayouts) {}
    virtual ~BaseProcessor() {}

protected:
    /** This is for backward compatibility with juce 6. Don't use in new processors */
    inline void addLegacyParameter (AudioProcessorParameter* param) {
        if (auto* hp = dynamic_cast<HostedAudioProcessorParameter*> (param))
            addHostedParameter (std::unique_ptr<HostedAudioProcessorParameter> (hp));
        jassertfalse;
    }
#if 0
    // Audio Processor Template
    virtual const String getName() const = 0;
    virtual StringArray getAlternateDisplayNames() const;
    virtual void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    
    virtual void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) = 0;
    virtual void processBlock (AudioBuffer<double>& buffer, idiBuffer& midiMessages);
    
    virtual void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midiMessages);
    virtual void processBlockBypassed (AudioBuffer<double>& buffer, MidiBuffer& midiMessages);
    
    virtual bool canAddBus (bool isInput) const                     { ignoreUnused (isInput); return false; }
    virtual bool canRemoveBus (bool isInput) const                  { ignoreUnused (isInput); return false; }
    virtual bool supportsDoublePrecisionProcessing() const;
    
    virtual double getTailLengthSeconds() const = 0;
    virtual bool acceptsMidi() const = 0;
    virtual bool producesMidi() const = 0;
    virtual bool supportsMPE() const                            { return false; }
    virtual bool isMidiEffect() const                           { return false; }
    virtual void reset();
    virtual void setNonRealtime (bool isNonRealtime) noexcept;
    
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual bool hasEditor() const = 0;
    
    virtual int getNumPrograms()        { return 1; };
    virtual int getCurrentProgram()     { return 1; };
    virtual void setCurrentProgram (int index) { ignoreUnused (index); };
    virtual const String getProgramName (int index) { ignoreUnused (index); }
    virtual void changeProgramName (int index, const String& newName) { ignoreUnused (index, newName); }
    virtual void getStateInformation (juce::MemoryBlock& destData) = 0;
    virtual void getCurrentProgramStateInformation (juce::MemoryBlock& destData);
    
    virtual void setStateInformation (const void* data, int sizeInBytes) = 0;
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
    virtual bool isBusesLayoutSupported (const BusesLayout&) const          { return true; }
    virtual bool canApplyBusesLayout (const BusesLayout& layouts) const     { return isBusesLayoutSupported (layouts); }
    virtual bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties);
#endif

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseProcessor)
};

} // namespace element
