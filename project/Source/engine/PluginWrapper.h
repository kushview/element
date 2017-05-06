/*
    PluginWrapper.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef EL_PLUGIN_WRAPPER_H
#define EL_PLUGIN_WRAPPER_H

#include "ElementApp.h"

namespace Element {

/** A simple wrapper processor.  This allows regular juce AudioProcessors
    like AudioUnits/VST etc etc, to be used in a Element::GraphProcessor
    and with the Element::PluginManager
*/
class PluginWrapper :  public Processor
{

public:

    
    inline PluginWrapper (AudioProcessor* plug)
    {
        jassert (plug != nullptr);
        proc = plug;
    }

    inline ~PluginWrapper()
    {
        proc = nullptr;
    }

    inline const String getName() const { return proc->getName(); }
    inline void prepareToPlay (double rate, int block) { proc->prepareToPlay (rate, block); }
    inline void releaseResources() { proc->releaseResources(); }
    inline void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) { proc->processBlock (buffer, midiMessages); }
    
    inline void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) { proc->processBlockBypassed (buffer, midiMessages); }
    
#if 0
    inline const String getInputChannelName (int channelIndex) const {
        return proc->busArrangement.inputBuses[channelIndex].name;
    }

    inline const String getOutputChannelName (int channelIndex) const {
        return proc->busArrangement.outputBuses[channelIndex].name;
    }
    
    inline bool isInputChannelStereoPair (int index) const {
        return proc->isInputChannelStereoPair (index);
    }
    
    inline bool isOutputChannelStereoPair (int index) const {
        return proc->isOutputChannelStereoPair (index);
    }
#endif

    inline double getTailLengthSeconds() const { return proc->getTailLengthSeconds(); }
    inline bool acceptsMidi() const { return proc->acceptsMidi(); }
    inline bool producesMidi() const { return proc->producesMidi(); }
    inline void reset() { proc->reset(); }
    inline AudioProcessorEditor* createEditor() { return proc->createEditor(); }
    inline bool hasEditor() const { return proc->hasEditor(); }
    inline int getNumParameters() { return proc->getNumParameters(); }
    inline const String getParameterName (int parameterIndex) { return proc->getParameterName (parameterIndex); }
    inline float getParameter (int parameterIndex) { return proc->getParameter (parameterIndex); }
    inline const String getParameterText (int parameterIndex) { return proc->getParameterText (parameterIndex); }
    inline String getParameterName (int parameterIndex, int maximumStringLength) { return proc->getParameterName (parameterIndex, maximumStringLength); }
    inline String getParameterText (int parameterIndex, int maximumStringLength) { return proc->getParameterText (parameterIndex, maximumStringLength); }
    inline int getParameterNumSteps (int parameterIndex) { return proc->getParameterNumSteps (parameterIndex); }
    inline float getParameterDefaultValue (int parameterIndex) { return proc->getParameterDefaultValue (parameterIndex); }
    inline String getParameterLabel (int index) const { return proc->getParameterLabel (index); }
    inline void setParameter (int parameterIndex, float newValue) { proc->setParameter(parameterIndex, newValue); }
    inline bool isParameterAutomatable (int parameterIndex) const { return proc->isParameterAutomatable (parameterIndex); }
    inline bool isMetaParameter (int parameterIndex) const { return proc->isMetaParameter (parameterIndex); }
    inline int getNumPrograms() { return proc->getNumPrograms(); }
    inline int getCurrentProgram() { return proc->getCurrentProgram(); }
    inline void setCurrentProgram (int index) { return proc->setCurrentProgram (index); }
    inline const String getProgramName (int index) { return proc->getProgramName (index); }
    inline void changeProgramName (int index, const String& newName) { proc->changeProgramName (index, newName); }
    inline void getStateInformation (MemoryBlock& destData) { proc->getStateInformation (destData); }
    inline void getCurrentProgramStateInformation (MemoryBlock& destData) { proc->getCurrentProgramStateInformation (destData); }
    inline void setStateInformation (const void* data, int sizeInBytes) { proc->setStateInformation (data, sizeInBytes); }
    inline void setCurrentProgramStateInformation (const void* data, int sizeInBytes) { proc->setCurrentProgramStateInformation (data, sizeInBytes); }
    inline void numChannelsChanged() { proc->numChannelsChanged(); }
    inline void addListener (AudioProcessorListener* newListener) { proc->addListener (newListener); }
    inline void removeListener (AudioProcessorListener* listenerToRemove) { proc->removeListener (listenerToRemove); }
    inline void setPlayHead (AudioPlayHead* newPlayHead) { proc->setPlayHead (newPlayHead); }

    inline void fillInPluginDescription (PluginDescription &d) const
    {
        if (AudioPluginInstance* plug = dynamic_cast<AudioPluginInstance*> (proc.get()))
            return plug->fillInPluginDescription (d);

        d.category = "Wrapper";
        d.descriptiveName = "Universal Plugin Wrapper";
        d.fileOrIdentifier = "internal://pluginWrapper";
        d.hasSharedContainer = false;
        d.isInstrument = proc->acceptsMidi();
        d.manufacturerName = "Element Project";
        d.name = proc->getName();
       #if 0
        d.numInputChannels = getNumInputChannels();
        d.numOutputChannels = getNumOutputChannels();
       #else
        d.numInputChannels  = getTotalNumInputChannels();
        d.numOutputChannels = getTotalNumOutputChannels();
       #endif
        d.pluginFormatName = "Internal";
        d.version = "0.0.1";
        d.uid = String("internal://pluginWrapper").hashCode();
    }

    inline uint32 getNumPorts()
    {
        return Processor::getNumPorts (proc);
    }
    
    inline uint32 getNumPorts (PortType type, bool isInput)
    {
        return Processor::getNumPorts (proc, type, isInput);
    }
    
    inline PortType getPortType (uint32 port)
    {
        return Processor::getPortType (proc, port);
    }
    
    inline bool isPortInput (uint32 port)
    {
        return Processor::isPortInput (proc, port);
    }
    
    inline AudioPluginInstance* getWrappedAudioPluginInstance() const { return dynamic_cast<AudioPluginInstance*> (proc.get()); }
private:
    ScopedPointer<AudioProcessor> proc;

};

}

#endif // ELEMENT_PLUGIN_WRAPPER_H
