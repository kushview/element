
#ifndef EL_MIDI_CLIP_H
#define EL_MIDI_CLIP_H

#include "element/Juce.h"

namespace Element {
    
    class MidiClip : public ClipModel
    {
    public:
        MidiClip();
        ~MidiClip();
        
        void addNotesTo (MidiMessageSequence&) const;
    };
}


#endif  // EL_MIDI_CLIP_H
