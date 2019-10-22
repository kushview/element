/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "session/MidiClip.h"
#include "session/NoteSequence.h"

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
