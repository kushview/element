#pragma once

#include "AtomTestNode.h"

namespace element {

/** Generates test MIDI events */
class MidiGeneratorNode : public AtomTestNode {
public:
    MidiGeneratorNode() : AtomTestNode (0, 1, 0, 0) {} // 1 MIDI output

    void render (RenderContext& rc) override
    {
        if (rc.midi.getNumBuffers() > 0) {
            auto* midi = rc.midi.getWriteBuffer (0);
            for (const auto& ev : eventsToGenerate)
                midi->addEvent (ev.message, ev.timestamp);
        }
    }

    void addEvent (const MidiMessage& msg, int timestamp = 0)
    {
        eventsToGenerate.add ({ msg, timestamp });
    }

    void clear()
    {
        eventsToGenerate.clear();
    }

private:
    struct Event {
        MidiMessage message;
        int timestamp;
    };
    Array<Event> eventsToGenerate;
};

} // namespace element
