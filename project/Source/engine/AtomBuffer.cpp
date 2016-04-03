/*
    AtomBuffer.cpp - This file is part of Element
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


#if 0
    class AtomBuffer::Data : public LV2Evbuf
    {
    public:

        Data() : LV2Evbuf (capacity, LV2Evbuf::lv2Atom, 0, 0)
        {

        }

        ~Data() { }

    };

    static LV2_Evbuf_Iterator
    findEventAfter (LV2Evbuf& buf, uint32 frame, uint32 subframe)
    {
        if (lv2_evbuf_get_size (buf) == 0)
            return lv2_evbuf_begin (buf);

        uint32 f, sf, t, sz;
        uint8_t* data = nullptr;

        LV2_EVBUF_FOREACH (buf, iter)
        {
            lv2_evbuf_get (iter, &f, &sf, &t, &sz, &data);
            if (f > frame)
                return iter;
        }

        return lv2_evbuf_end (buf);
    }

    AtomBuffer::AtomBuffer (LV2_URID_Map* map, uint32 capacity = 4096)
        : Buffer(Buffer::atomData),
                 evbuf (capacity, Element::LV2Evbuf::lv2Atom,
                 map->map (map->handle, LV2_ATOM__Sequence),
                 map->map (map->handle, LV2_ATOM__Chunk)),
          MidiEvent (map->map (map->handle, LV2_MIDI__MidiEvent))
    { }

    void
    AtomBuffer::addEvent (uint32 frame, uint32 subframe, uint32 size, uint32 type, const uint8* data)
    {
        jassert (type != 0);

        LV2_Evbuf_Iterator iter = findEventAfter (evbuf, frame, subframe);

        if (iter.offset != lv2_evbuf_end(evbuf).offset)
        {
            uint8_t* buf = (uint8_t*) evbuf.getBuffer();
            uint32 evsize = padSize (sizeof (LV2_Atom_Event) + size);
            memmove (buf + iter.offset + evsize,
                     buf + iter.offset,
                     padSize (sizeof(LV2_Atom_Event) + evbuf.getSize() - iter.offset));
        }

        lv2_evbuf_write (&iter, frame, subframe, type, size, (uint8_t*) data);
    }

    void
    AtomBuffer::addEvent (const MidiMessage& midi, uint32 frame, uint32 subframe)
    {
        addEvent (frame, subframe, (uint32) midi.getRawDataSize(), MidiEvent, midi.getRawData());
    }

    void
    AtomBuffer::addEvents (const MidiBuffer& midi)
    {
        const uint8_t* data;
        int frame, size;

        MidiBuffer::Iterator iter (midi);
        while (iter.getNextEvent (data, size, frame))
            addEvent ((uint32) frame, 0, (uint32) size, MidiEvent, data);
    }

    void
    AtomBuffer::addEvents (AtomBuffer& other, int nframes)
    {
        addEvents (other, uint32 (nframes));
    }

    void
    AtomBuffer::swapWith (AtomBuffer& other) {

    }

    void
    AtomBuffer::clear (bool isInput)
    {
        evbuf.reset (isInput);
    }

    void
    AtomBuffer::clear ()
    {
        evbuf.reset();
    }

    void
    AtomBuffer::removeUntil (uint32 frame)
    {
        evbuf.removeUntil (frame);
    }

    void*
    AtomBuffer::getBuffer()
    {
        return evbuf.getBuffer();
    }

    void AtomBuffer::toggleFlow (bool isInput = true)
    {
        lv2_evbuf_toggle_flow (evbuf, isInput);
    }

    uint32 AtomBuffer::size() { return data->getSize(); }

    uint32
    AtomBuffer::padSize (uint32 size)
    {
        return (size + 7) & (~7);
    }
#endif
