
#include "session/MidiClip.h"

namespace Element {
    
    MidiClip::MidiClip()
        : ClipModel (Slugs::clip)
    {
        setMissingProperties();
        objectData.setProperty (Slugs::type, "midi", nullptr);
        NoteSequence notes (objectData.getOrCreateChildWithName ("notes", nullptr));
    }
    
    MidiClip::~MidiClip() { }
}
