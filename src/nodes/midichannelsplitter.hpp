// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/midipipe.hpp>
#include "nodes/midifilter.hpp"
#include "nodes/baseprocessor.hpp"

namespace element {

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

    inline void render (RenderContext& rc) override
    {
        if (rc.midi.getNumBuffers() < 16)
        {
            if (! assertedLowChannels)
            {
                assertedLowChannels = true;
                jassertfalse;
            }

            rc.midi.clear (0, rc.audio.getNumSamples());
            return;
        }

        buffers[0] = &tempMidi;
        for (int ch = 1; ch < 16; ++ch)
        {
            buffers[ch] = rc.midi.getWriteBuffer (ch);
            buffers[ch]->clear();
        }

        MidiBuffer& input (*rc.midi.getWriteBuffer (0));
        for (auto m : input)
        {
            auto msg = m.getMessage();
            if (msg.getChannel() <= 0)
                continue;
            buffers[msg.getChannel() - 1]->addEvent (msg, m.samplePosition);
        }

        input.swapWith (tempMidi);
        tempMidi.clear();
    }

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_NODE_ID_MIDI_CHANNEL_SPLITTER;
        desc.name = "MIDI Channel Splitter";
        desc.descriptiveName = "MIDI Channel Splitter";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = EL_NODE_FORMAT_NAME;
        desc.version = "1.0.0";
        desc.uniqueId = EL_NODE_UID_MIDI_CHANNEL_SPLITTER;
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

} // namespace element