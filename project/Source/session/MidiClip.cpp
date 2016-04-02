
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
    
    void MidiClip::addNotesTo (MidiMessageSequence& seq) const
    {
        const ValueTree notes (objectData.getChildWithName ("notes"));
        for (int i = 0; i < notes.getNumChildren(); ++i)
        {
            const Note note (notes.getChild (i));
            MidiMessage on, off;
            note.getMidi (on, off);
            seq.addEvent(on);
            seq.addEvent(off);
            seq.updateMatchedPairs();
        }
    }
}
