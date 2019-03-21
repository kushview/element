#pragma once

#include "engine/nodes/MidiFilterNode.h"
#include "engine/MidiPipe.h"
#include "engine/nodes/BaseProcessor.h"

namespace Element {

class MidiChannelSplitterNode : public MidiFilterNode
{
public:
    MidiChannelSplitterNode() 
        : MidiFilterNode (0)
    {
        jassert (metadata.hasType (Tags::node));
        metadata.setProperty (Tags::format, "Element", nullptr);
        metadata.setProperty (Tags::identifier, "element.midiChannelSplitter", nullptr);
    }

    ~MidiChannelSplitterNode() { }

    void setState (const void* data, int size) override { ignoreUnused (data, size); }
    void getState (MemoryBlock& block) override { ignoreUnused (block); }

    void prepareToRender (double sampleRate, int maxBufferSize) override { ignoreUnused (sampleRate, maxBufferSize); }
    void releaseResources() override { }

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
        MidiMessage msg; int frame = 0;

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
        desc.fileOrIdentifier   = EL_INTERNAL_ID_MIDI_CHANNEL_SPLITTER;
        desc.name               = "MIDI Channel Splitter";
        desc.descriptiveName    = "MIDI Channel Splitter";
        desc.numInputChannels   = 0;
        desc.numOutputChannels  = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
        desc.uid                = EL_INTERNAL_UID_MIDI_CHANNEL_SPLITTER;
    }

protected:
    bool assertedLowChannels = false;
    bool createdPorts = false;
    MidiBuffer* buffers [16];
    MidiBuffer tempMidi;

    inline void createPorts() override
    {
        if (createdPorts)
            return;

        ports.clearQuick();
        ports.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        for (int ch = 1; ch <= 16; ++ch)
        {
            String symbol = "midi_out_"; symbol << ch;
            String name = "Ch. "; name << ch;
            ports.add (PortType::Midi, ch, ch - 1, symbol, name, false);
        }
        createdPorts = true;
    }
};

}