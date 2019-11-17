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

#define EL_INTERNAL_FORMAT_NAME                 "Element"

#define EL_INTERNAL_ID_AUDIO_FILE_PLAYER        "element.audioFilePlayer"
#define EL_INTERNAL_ID_AUDIO_MIXER              "element.audioMixer"
#define EL_INTERNAL_ID_AUDIO_ROUTER             "element.audioRouter"
#define EL_INTERNAL_ID_COMB_FILTER              "element.comb"
#define EL_INTERNAL_ID_CHANNELIZE               "element.channelize"
#define EL_INTERNAL_ID_GRAPH                    "element.graph"
#define EL_INTERNAL_ID_MEDIA_PLAYER             "element.mediaPlayer"
#define EL_INTERNAL_ID_MIDI_CHANNEL_MAP         "element.midiChannelMap"
#define EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER    "element.midiChannelSplitter"
#define EL_INTERNAL_ID_MIDI_PROGRAM_MAP         "element.programChangeMap"
#define EL_INTERNAL_ID_MIDI_SEQUENCER           "element.midiSequencer"
#define EL_INTERNAL_ID_PLACEHOLDER              "element.placeholder"
#define EL_INTERNAL_ID_REVERB                   "element.reverb"
#define EL_INTERNAL_ID_WET_DRY                  "element.wetDry"
#define EL_INTERNAL_ID_MIDI_INPUT_DEVICE        "element.midiInputDevice"
#define EL_INTERNAL_ID_MIDI_OUTPUT_DEVICE       "element.midiOutputDevice"
#define EL_INTERNAL_ID_MIDI_MONITOR             "element.midiMonitor"
#define EL_INTERNAL_ID_OSC_RECEIVER             "element.oscReceiver"
#define EL_INTERNAL_ID_OSC_SENDER               "element.oscSender"
#define EL_INTERNAL_ID_EQ_FILTER                "element.eqfilt"
#define EL_INTERNAL_ID_FREQ_SPLITTER            "element.freqsplit"
#define EL_INTERNAL_ID_LUA                      "element.lua"

#define EL_INTERNAL_UID_AUDIO_FILE_PLAYER        1000
#define EL_INTERNAL_UID_AUDIO_MIXER              1001
#define EL_INTERNAL_UID_AUDIO_ROUTER             1002
#define EL_INTERNAL_UID_COMB_FILTER              1003
#define EL_INTERNAL_UID_CHANNELIZE               1004
#define EL_INTERNAL_UID_GRAPH                    1005
#define EL_INTERNAL_UID_MEDIA_PLAYER             1006
#define EL_INTERNAL_UID_MIDI_CHANNEL_MAP         1007
#define EL_INTERNAL_UID_MIDI_CHANNEL_SPLITTER    1008
#define EL_INTERNAL_UID_MIDI_PROGRAM_MAP         1009
#define EL_INTERNAL_UID_MIDI_SEQUENCER           1010
#define EL_INTERNAL_UID_PLACEHOLDER              1011
#define EL_INTERNAL_UID_REVERB                   1012
#define EL_INTERNAL_UID_WET_DRY                  1013
#define EL_INTERNAL_UID_MIDI_INPUT_DEVICE        1014
#define EL_INTERNAL_UID_MIDI_OUTPUT_DEVICE       1015
#define EL_INTERNAL_UID_MIDI_MONITOR             1016
#define EL_INTERNAL_UID_OSC_RECEIVER             1017
#define EL_INTERNAL_UID_OSC_SENDER               1018
#define EL_INTERNAL_UID_EQ_FILTER                1019
#define EL_INTERNAL_UID_FREQ_SPLITTER            1020
#define EL_INTERNAL_UID_LUA                      1021

namespace Element {

class BaseProcessor : public AudioPluginInstance
{
public:
    BaseProcessor() : AudioPluginInstance() { }
    BaseProcessor (const BusesProperties& ioLayouts)
        : AudioPluginInstance (ioLayouts) { }
    virtual ~BaseProcessor() { }

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

}
