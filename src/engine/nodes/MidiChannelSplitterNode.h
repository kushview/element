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

#include "engine/nodes/MidiFilterNode.h"
#include "engine/midipipe.hpp"
#include "engine/nodes/BaseProcessor.h"

namespace Element {

class MidiChannelSplitterNode : public MidiFilterNode
{
public:
    MidiChannelSplitterNode()
        : MidiFilterNode (0) {}

    ~MidiChannelSplitterNode() {}

    void setState (const void* data, int size) override { ignoreUnused (data, size); }
    void getState (MemoryBlock& block) override { ignoreUnused (block); }

    void prepareToRender (double sampleRate, int maxBufferSize) override { ignoreUnused (sampleRate, maxBufferSize); }
    void releaseResources() override {}

    inline void render (AudioSampleBuffer& audio, MidiPipe& midi) override
    {
        if (midi.getNumBuffers() < 16)
        {
            if (! assertedLowChannels)
            {
                assertedLowChannels = true;
                jassertfalse;
            }

            midi.clear (0, audio.getNumSamples());
            return;
        }

        buffers[0] = &tempMidi;
        for (int ch = 1; ch < 16; ++ch)
        {
            buffers[ch] = midi.getWriteBuffer (ch);
            buffers[ch]->clear();
        }

        MidiBuffer& input (*midi.getWriteBuffer (0));
        MidiBuffer::Iterator iter (input);
        MidiMessage msg;
        int frame = 0;

        while (iter.getNextEvent (msg, frame))
        {
            if (msg.getChannel() <= 0)
                continue;
            buffers[msg.getChannel() - 1]->addEvent (msg, frame);
        }

        input.swapWith (tempMidi);
        tempMidi.clear();
    }

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER;
        desc.name = "MIDI Channel Splitter";
        desc.descriptiveName = "MIDI Channel Splitter";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = "Element";
        desc.pluginFormatName = EL_INTERNAL_FORMAT_NAME;
        desc.version = "1.0.0";
        desc.uniqueId = EL_INTERNAL_UID_MIDI_CHANNEL_SPLITTER;
    }

protected:
    bool assertedLowChannels = false;
    bool createdPorts = false;
    MidiBuffer* buffers[16];
    MidiBuffer tempMidi;

    inline void refreshPorts() override
    {
        if (createdPorts)
            return;

        PortList newPorts;
        newPorts.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        for (int ch = 1; ch <= 16; ++ch)
        {
            String symbol = "midi_out_";
            symbol << ch;
            String name = "Ch. ";
            name << ch;
            newPorts.add (PortType::Midi, ch, ch - 1, symbol, name, false);
        }
        createdPorts = true;
        setPorts (newPorts);
    }
};

} // namespace Element