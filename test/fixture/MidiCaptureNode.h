#pragma once

#include "fixture/TestNode.h"

namespace element {

/** Captures MIDI events for verification */
class MidiCaptureNode : public TestNode {
public:
    MidiCaptureNode() : TestNode (0, 0, 1, 0) {} // 1 MIDI input

    void render (RenderContext& rc) override
    {
        eventCount = 0;
        
        if (rc.midi.getNumBuffers() > 0)
        {
            auto* midiBuf = rc.midi.getWriteBuffer (0);
            eventCount = midiBuf->getNumEvents();
        }
    }

    int getEventCount() const { return eventCount; }
    void reset() { eventCount = 0; }

private:
    int eventCount = 0;
};

} // namespace element
