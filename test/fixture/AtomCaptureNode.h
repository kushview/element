#pragma once

#include "AtomTestNode.h"
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>

namespace element {

/** Captures atom events for verification */
class AtomCaptureNode : public AtomTestNode {
public:
    AtomCaptureNode() : AtomTestNode (0, 0, 1, 0) {} // 1 Atom input

    void render (RenderContext& rc) override
    {
        eventCount = 0;
        
        if (rc.atom.size() > 0)
        {
            auto* atomBuf = rc.atom.readBuffer (0);
            auto seq = atomBuf->sequence();
            
            LV2_ATOM_SEQUENCE_FOREACH (seq, ev)
            {
                eventCount++;
            }
        }
    }

    int getEventCount() const { return eventCount; }
    void reset() { eventCount = 0; }

private:
    int eventCount = 0;
};

} // namespace element
