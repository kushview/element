#pragma once

#include "engine/nodes/MidiFilterNode.h"

namespace Element {

class MidiChannelSplitterNode : public MidiFilterNode
{
public:
    MidiChannelSplitterNode() : MidiFilterNode (KV_INVALID_NODE) { }
    ~MidiChannelSplitterNode() { }

protected:
    inline void createPorts() override
    {
        if (ports.size() > 0)
            return;

        ports.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        for (int ch = 1; ch <= 16; ++ch)
        {
            String symbol = "midi_out_"; symbol << ch;
            String name = "Ch. "; name << ch;
            ports.add (PortType::Midi, ch, ch - 1, symbol, name, false);
        }
    }

    void render (AudioBuffer<float>&, OwnedArray<MidiBuffer>&) {
        
    }
};

}