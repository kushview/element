/*
    NoteSequence.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_NOTESEQUENCE_H
#define ELEMENT_NOTESEQUENCE_H

#include "ElementApp.h"
#include "session/Note.h"

namespace Element {
/** A multi-purpose container for notes. This emits an add/removed signal
    as well as a ValueTree listener interface. */
class NoteSequence :  public ObjectModel
{
public:
    
    /** Create an empty sequence */
    NoteSequence() : ObjectModel ("noteSequence") { }
    
    /** Create an empty sequence */
    NoteSequence (const ValueTree& data)
        : ObjectModel (data)
    { }
    
    NoteSequence (const NoteSequence& o)
        : ObjectModel (o.node())
    { }
    
    ~NoteSequence() { }
    
    inline void
    getKeysWithEvents (BigInteger& keys)
    {
        for (int i = node().getNumChildren(); --i >= 0;)
        {
            Note n (node().getChild (i));
            keys.setBit (n.keyId(), true);
        }
    }
    
    /** Add a new note to the sequence */
    inline Note
    addNote (int note, double beat, double length = 1.0f,
             int channel = 1, float velocity = 0.8f)
    {
        Note n (note, beat, length, channel, velocity);
        node().addChild (n.node(), -1, nullptr);
        return n;
    }
    
    inline Note
    addNote()
    {
        Note note (0, 0.f, 0.f, 0);
        node().addChild (note.node(), -1, nullptr);
        return note;
    }
    
    /** Add a note from a value tree object */
    Note addNote (const ValueTree& tree);
    
    inline Note addNote (const MidiMessage& on, const MidiMessage& off)
    {
        Note note (on.getNoteNumber(), on.getTimeStamp(),
                   off.getTimeStamp() - on.getTimeStamp(),
                   on.getChannel(), on.getFloatVelocity());
        objectData.addChild (note.node(), -1, nullptr);
        return note;
    }
    
    
    inline void
    addMidiMessageSequence (const MidiMessageSequence& mseq)
    {
        const int32 numEvs = mseq.getNumEvents();
        for (int32 i = 0; i < numEvs; ++i)
            if (MidiMessageSequence::MidiEventHolder* mh = mseq.getEventPointer (i))
                if (mh->message.isNoteOn()&& mh->noteOffObject != nullptr)
                    addNote (mh->message, mh->noteOffObject->message);
    }
    
    
    /** Remove a note from the sequence */
    inline void removeNote (const Note& note)
    {
        if (note.node().isAChildOf (objectData))
            objectData.removeChild (note.node(), nullptr);
    }
    
    /** Add a value tree listener
     
        This is so things that don't know about this specific class
        can still be listeners of the sequence/notes 
     */
    inline void addValueListener (ValueTree::Listener* client) {
        objectData.addListener (client);
    }
    
    /** Remove a value tree listener */
    inline void removeValueListener (ValueTree::Listener* client) {
        objectData.removeListener (client);
    }
    
    /** Add a note from Midi */
    inline Note addMidi (const MidiMessage& msg, double len = 1.0f)
    {
        const Note n (Note::make (msg, len));
        objectData.addChild (n.node(), -1, nullptr);
        return n;
    }
    
    inline bool operator== (const NoteSequence& seq) const { return seq.node() == this->node(); }
    inline bool operator!= (const NoteSequence& seq) const { return seq.node() != this->node(); }
    
    inline NoteSequence& operator= (const NoteSequence& o) {
        setNodeData (o.node());
        return *this;
    }
    
    inline void clear (UndoManager* u = nullptr) {
        node().removeAllChildren (u);
    }
    
    /** This will return Shuttle::PPQ unless specified otherwise
        by setting the property "ppq" to and integer value
         
        The easiest thing to do when working with foreign MIDI sources
        is to read tick values from the source, scale them with,
        Shuttle::scaledTick, then add notes to the sequences with new tick
        values.
     */
    int32 ppq() const;
    
private:
    friend class Note;
};

}

#endif /* ELEMENT_NOTESEQUENCE_H */
