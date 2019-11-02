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

#include "engine/nodes/AudioProcessorNode.h"
#include "engine/nodes/BaseProcessor.h"
#include "engine/GraphProcessor.h"
#include "engine/nodes/MidiDeviceProcessor.h"


namespace Element {

static void setAudioProcessorNodePropertiesFrom (const PluginDescription& pd, ValueTree& p)
{
    p.setProperty (Tags::format,        pd.pluginFormatName, nullptr);
    p.setProperty (Tags::identifier,    pd.fileOrIdentifier, nullptr);
    if (pd.pluginFormatName == "Element" && pd.fileOrIdentifier == EL_INTERNAL_ID_GRAPH)
        p.setProperty (Tags::type, Tags::graph.toString(), nullptr);
}

void AudioProcessorNode::prepareToRender (double sampleRate, int maxBufferSize) 
{ 
    if (! proc)
    {
        jassertfalse;
        return;
    }

    proc->setRateAndBufferSizeDetails (sampleRate, maxBufferSize);
    proc->prepareToPlay (sampleRate, maxBufferSize);
}

void AudioProcessorNode::releaseResources() 
{ 
    if (! proc)
    {
        jassertfalse;
        return;
    }

    proc->releaseResources();
}

void AudioProcessorNode::EnablementUpdater::handleAsyncUpdate()
{
    DBG("[EL] AudioProcessorNode::EnablementUpdater::handleAsyncUpdate()");
    node.setEnabled (! node.isEnabled());
}

AudioProcessorNode::AudioProcessorNode (uint32 nodeId, AudioProcessor* processor)
    : GraphNode (nodeId),
      enablement (*this)
{
    proc = processor;
    jassert (proc != nullptr);
    setLatencySamples (proc->getLatencySamples());
    setName (proc->getName());
    
    if (auto* instance = dynamic_cast<AudioPluginInstance*> (proc.get()))
    {
        setAudioProcessorNodePropertiesFrom (instance->getPluginDescription(), metadata);
    }
    else
    {
        jassertfalse; // need a way to identify normal audio processors
    }
}

AudioProcessorNode::~AudioProcessorNode()
{
    enablement.cancelPendingUpdate();
    pluginState.reset();
    proc = nullptr;
}

void AudioProcessorNode::getState (MemoryBlock& block)
{
    if (proc)
        proc->getStateInformation (block);
}

void AudioProcessorNode::setState (const void* data, int size)
{
    if (proc)
        proc->setStateInformation (data, size);
}

void AudioProcessorNode::createPorts()
{
    kv::PortList newPorts;

    auto* const midiDevice  = dynamic_cast<MidiDeviceProcessor*> (proc.get());
    const bool isMidiDevice = nullptr != midiDevice;
    const bool isMidiDeviceInput  = isMidiDevice && midiDevice->isInputDevice();
    const bool isMidiDeviceOutput = isMidiDevice && midiDevice->isOutputDevice();
    ignoreUnused (isMidiDeviceInput, isMidiDeviceOutput);
    
    int index = 0, channel = 0, busIdx = 0;
    for (busIdx = 0; busIdx < proc->getBusCount(true); ++busIdx)
    {
        auto* const bus = proc->getBus (true, busIdx);
        for (int busCh = 0; busCh < bus->getNumberOfChannels(); ++busCh)
        {
            String name = bus->getName(); name << " " << int (channel + 1);
            String symbol = "audio_in_"; symbol << int (channel + 1);
            newPorts.add (PortType::Audio, index, channel, symbol, name, true);
            ++index; ++channel;
            jassert (! isMidiDevice);
        }
    }
    jassert (channel == proc->getTotalNumInputChannels());

    channel = 0;
    for (busIdx = 0; busIdx < proc->getBusCount (false); ++busIdx)
    {
        auto* const bus = proc->getBus (false, busIdx);
        for (int busCh = 0; busCh < bus->getNumberOfChannels(); ++busCh)
        {
            String name = bus->getName(); name << " " << int (channel + 1);
            String symbol = "audio_out_"; symbol << int (channel + 1);
            newPorts.add (PortType::Audio, index, channel, symbol, name, false);
            ++index; ++channel;
            jassert(! isMidiDevice);
        }
    }
    jassert (channel == proc->getTotalNumOutputChannels());

    const auto& params = proc->getParameters();
    for (int i = 0; i < params.size(); ++i)
    {
        auto* const param = params.getUnchecked (i);
        String symbol = "control_"; symbol << i;
        newPorts.add (PortType::Control, index, i, 
                      symbol, param->getName (32),
                      true);
        ++index;
    }

    if (proc->acceptsMidi())
    {
        newPorts.add (PortType::Midi, index, 0, "midi_in_0", "MIDI", true);
        ++index;
    }
    
    if (proc->producesMidi())
    {
        if (isMidiDevice)
        {
            jassert (isMidiDeviceInput);
        }
        newPorts.add (PortType::Midi, index, 0, "midi_out_0", "MIDI", false);
        ++index;
    }

    jassert (index == newPorts.size());
    
    ports.swapWith (newPorts);
}

}
