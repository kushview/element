/*
    MidiDataBase.cpp - This file is part of Element
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

MidiDataBase::MidiDataBase ()
{
    eventMap.set (0, nullptr);
}

MidiDataBase::~MidiDataBase () { }

bool MidiDataBase::addNote (const Note& note)
{
    MidiMessage msgOn, msgOff;
    note.getMidi (msgOn, msgOff);

    lock();
    EventHolder* event = midi.addEvent (msgOn);
    midi.updateMatchedPairs();
    unlock();

    int32 eventId = note.eventId();

    if (event != nullptr)
    {
        lock();
        midi.addEvent (msgOff);
        midi.updateMatchedPairs();
        unlock();

        if (eventId <= 0)
            eventId = nextId();

        eventMap.set (eventId, event);
        note.setEventId (eventId);
    }
    else
    {
        note.setEventId (0);
    }

    return note.eventId() > 0;
}

void MidiDataBase::buildHashTable (ValueTree&) { }

bool MidiDataBase::editNote (const Note& note)
{
    if (note.eventId() <= 0)
        return false;

    EventHolder* down (eventMap [note.eventId()]);
    if (! down) {
        return false;
    }

    MidiMessage* noteOn = &down->message;
    if (! noteOn->isNoteOn())
        return false;

    EventHolder* up = down->noteOffObject;
    if (! up)
        return false;

    MidiMessage* noteOff = &up->message;
    if (! noteOff)
        return false;

    lock();
    if (note.keyId() != noteOn->getNoteNumber())
    {
        noteOn->setNoteNumber (note.keyId());
        noteOff->setNoteNumber (noteOn->getNoteNumber());
    }

    if (note.channel() != noteOn->getChannel())
    {
        noteOn->setChannel (note.channel());
        noteOff->setChannel (noteOn->getChannel());
    }
    unlock();

    bool editResult = true;

    if (note.tickStart() != noteOn->getTimeStamp() ||
        note.tickEnd() != noteOff->getTimeStamp())
    {
        editResult = false;

        lock();
        noteOn->setTimeStamp (note.tickStart());
        noteOff->setTimeStamp (note.tickEnd());
        midi.updateMatchedPairs();
        midi.sort();
        unlock();

        editResult = true;
    }

    return editResult;
}

bool MidiDataBase::expireNote (const Note& note)
{
    const int32 evid = note.eventId();
    bool res = true;

    if (evid > 0)
    {
        if (eventMap.contains (evid))
        {
            res = removeNote (note);
            if (res)
                expiredIds.addIfNotAlreadyThere (evid);
        }
    } else {

    }

    return res;
}

int32 MidiDataBase::nextId()
{
    if (expiredIds.size() > 0)
    {
        int32 res = expiredIds.getLast();
        expiredIds.removeLast();
        return res;
    }

    return ++lastEventId;
}


bool MidiDataBase::removeNote (const Note& note)
{
    const int32 evid = note.eventId();
    if (EventHolder* ev = eventMap [evid])
    {
        const int evIndex = midi.getIndexOf (ev);
        eventMap.remove (evid);

        lock();
        midi.deleteEvent (evIndex, true);
        midi.updateMatchedPairs();
        unlock();

        return true;
    }

    return false;
}
