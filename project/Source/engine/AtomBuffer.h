/*
    AtomBuffer.h - This file is part of Element
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

#ifndef ELEMENT_ATOMBUFFER_H
#define ELEMENT_ATOMBUFFER_H

class AtomBuffer {
public:
    AtomBuffer (LV2_URID_Map* map, uint32 capacity = 4096);

    void addEvent (uint32_t frame, uint32_t subframe, uint32_t size, uint32_t type, const uint8* data);
    void addEvent (const MidiMessage& midi, uint32 frame, uint32 subframe);
    void addEvents (const MidiBuffer& midi);
    void addEvents (AtomBuffer& other, int nframes);

    void swapWith(AtomBuffer&) { jassertfalse;  }

    void clear (bool isInput);
    void clear ();
    void removeUntil (uint32 frame);

    void* getBuffer();

    uint32 size() const;

private:
    class Data;
    Data* data;

    LV2_URID MidiEvent;

    uint32 padSize (uint32_t size);
};

#endif /* ELEMENT_ATOMBUFFER_H */
