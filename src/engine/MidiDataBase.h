/*
    MidiDataBase.h - This file is part of Element
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

#ifndef EL_MIDI_DATA_BASE_H
#define EL_MIDI_DATA_BASE_H

class Note;

class MidiDataBase :  public ClipData,
                      public AtomicLock
{
public:
    typedef MidiMessageSequence::MidiEventHolder EventHolder;

    MidiDataBase();
    virtual ~MidiDataBase();

    /** Add note on and off to the MidiMessageSequence field

        @param note The note to add
        @param eventId the desired id for the new added note @see editNote
               for an example of how to use this
    */
    bool addNote (const Note& note);

    /** Update a Midi event with this note */
    bool editNote (const Note& note);

    /** Remove and clear the eventId for @c note */
    bool expireNote (const Note& note);

    /** Remove an Event from the Sequence */
    bool removeNote (const Note& note);

private:
    int32 lastEventId;
    Array<int32> expiredIds;
    HashMap<int32, EventHolder*> eventMap;

    /** @internal */
    void buildHashTable (ValueTree& noteData);

    /** @internal */
    int32 nextId();
};

#endif // EL_MIDI_DATA_BASE_H
