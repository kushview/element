// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <cassert>

#include <element/atombuffer.hpp>
#include <element/juce/audio_basics.hpp>

#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>

namespace element {

AtomBuffer::AtomBuffer()
    : _data (8192)
{
    _capacity = _data.size();
    _ptrs.raw = _data.data();
    clear();
}

AtomBuffer::~AtomBuffer()
{
    _capacity = 0;
    _ptrs.raw = nullptr;
    _data.reset();
}

void AtomBuffer::setTypes (LV2_URID_Map* map)
{
    setTypes (map->map (map->handle, LV2_ATOM__Sequence),
              map->map (map->handle, LV2_MIDI__MidiEvent));
}

void AtomBuffer::setTypes (uint32_t as, uint32_t me)
{
    _ptrs.atom->type = as;
    MidiEvent = me;
}

void AtomBuffer::clear()
{
    _ptrs.atom->size = sizeof (LV2_Atom_Sequence_Body);
}

void AtomBuffer::prepare()
{
    _ptrs.atom->size = _capacity - sizeof (LV2_Atom_Sequence_Body);
}

void AtomBuffer::insert (int64_t frames, uint32_t size, uint32_t type, const void* data)
{
    if (sizeof (LV2_Atom) + _ptrs.atom->size + lv2_atom_pad_size (size) > _capacity)
        return;

    const auto size_needed = lv2_atom_pad_size (sizeof (LV2_Atom_Event) + size);
    LV2_Atom_Event* ev = (LV2_Atom_Event*) ((uint8_t*) _ptrs.seq + lv2_atom_total_size (&_ptrs.seq->atom));

    LV2_ATOM_SEQUENCE_FOREACH (_ptrs.seq, i)
    {
        if (i->time.frames > frames)
        {
            std::memmove (((uint8_t*) i) + size_needed,
                          i,
                          (uint8_t*) ev - (uint8_t*) i);
            ev = i;
            break;
        }
    }

    assert (ev != nullptr);

    ev->time.frames = frames;
    ev->body.size = size;
    ev->body.type = type;
    std::memcpy (ev + 1, data, size);

    _ptrs.atom->size += size_needed;
}

void AtomBuffer::insert (juce::MidiMessage& msg, int frame)
{
    insert (frame,
            static_cast<uint32_t> (msg.getRawDataSize()),
            MidiEvent,
            msg.getRawData());
}

void AtomBuffer::add (const AtomBuffer& other)
{
    LV2_ATOM_SEQUENCE_FOREACH (other._ptrs.seq, i)
    {
        insert (i->time.frames,
                i->body.size,
                i->body.type,
                LV2_ATOM_BODY (&i->body));
    }
}

void AtomBuffer::add (juce::MidiBuffer& midi)
{
    for (const auto& i : midi)
    {
        insert (i.samplePosition,
                static_cast<uint32_t> (i.numBytes),
                MidiEvent,
                i.data);
    }
}

} // namespace element
