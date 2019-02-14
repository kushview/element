#pragma once

#include "engine/nodes/MidiFilterNode.h"
#include "engine/MidiPipe.h"
#include "engine/BaseProcessor.h"

namespace Element {

class ProgramChangeMapNode : public MidiFilterNode
{
public:
    ProgramChangeMapNode() : MidiFilterNode (0)
    {
        jassert (metadata.hasType (Tags::node));
        metadata.setProperty (Tags::format, "Element", nullptr);
        metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_PROGRAM_CHANGE_MAP, nullptr);
    }

    ~ProgramChangeMapNode() { }

    inline void render (AudioSampleBuffer& audio, MidiPipe& midi) override
    {
        ignoreUnused (audio, midi);
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
        ports.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
        createdPorts = true;
    }
};

}
