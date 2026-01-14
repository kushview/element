// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <lv2/atom/util.h>

#include "engine/portbuffer.hpp"

namespace element {

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")

static uint32_t portBufferPadSize (uint32_t size)
{
    return (size + 7) & (~7);
}

PortBuffer::PortBuffer (bool inputPort, uint32 portType, uint32 dataType, uint32 bufferSize)
    : type (portType),
      capacity (std::max (sizeof (float), (size_t) bufferSize)),
      bufferType (dataType),
      input (inputPort)
{
    data.reset (new uint8[capacity]);
    memset (data.get(), 0, capacity);

    if (type == PortType::Atom)
    {
        buffer.atom = (LV2_Atom*) data.get();
    }
    else if (type == PortType::Event)
    {
        buffer.event = (LV2_Event_Buffer*) data.get();
    }
    else if (type == PortType::Audio)
    {
        buffer.audio = (float*) data.get();
    }
    else if (type == PortType::Control)
    {
        buffer.control = (float*) data.get();
        (*buffer.control) = 0.f;
    }
    else if (type == PortType::CV)
    {
        buffer.cv = (float*) data.get();
        (*buffer.cv) = 0.f;
    }
    else
    {
        // trying to use an unsupported buffer type
        jassertfalse;
    }

    reset();
}

PortBuffer::~PortBuffer()
{
    buffer.atom = nullptr;
    data.reset();
}

void PortBuffer::resize (uint32_t newSize)
{
    // Ensure minimum size
    newSize = std::max (sizeof (float), (size_t) newSize);

    // If the size hasn't changed, nothing to do
    if (referenced || type != PortType::Atom || newSize == capacity || buffer.atom == nullptr)
    {
        return;
    }

    // Store the current atom size and data
    const uint32_t currentAtomSize = buffer.atom->size;
    const uint32_t atomHeaderSize = sizeof (LV2_Atom);
    const uint32_t totalCurrentSize = atomHeaderSize + currentAtomSize;

    // Allocate new buffer
    std::unique_ptr<uint8[]> newData (new uint8[newSize]);
    memset (newData.get(), 0, newSize);

    // Copy existing data, but don't exceed the new buffer size
    const uint32_t copySize = std::min (totalCurrentSize, newSize);
    memcpy (newData.get(), data.get(), copySize);

    // Update capacity and swap buffers
    capacity = newSize;
    data = std::move (newData);

    // Update buffer pointer to new location
    buffer.atom = (LV2_Atom*) data.get();

    // If the new size is smaller than the current atom data,
    // we need to truncate the atom size
    if (newSize < totalCurrentSize)
    {
        buffer.atom->size = newSize - atomHeaderSize;
    }
}

float PortBuffer::getValue() const
{
    jassert (type == PortType::Control);
    jassert (buffer.control != nullptr);
    return *buffer.control;
}

void PortBuffer::setValue (float newValue)
{
    jassert (type == PortType::Control);
    jassert (buffer.control != nullptr);
    (*buffer.control) = newValue;
}

bool PortBuffer::addEvent (int64 frames, uint32 size, uint32 bodyType, const uint8* data)
{
    if (isSequence())
    {
        if (sizeof (LV2_Atom) + buffer.atom->size + lv2_atom_pad_size (size) > capacity)
            return false;

        LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*) buffer.atom;
        LV2_Atom_Event* ev = (LV2_Atom_Event*) ((uint8*) seq + lv2_atom_total_size (&seq->atom));

        ev->time.frames = frames;
        ev->body.size = size;
        ev->body.type = bodyType;
        memcpy (ev + 1, data, size);

        buffer.atom->size += sizeof (LV2_Atom_Event) + lv2_atom_pad_size (size);
        return true;
    }
    else if (isEvent())
    {
        if (buffer.event->capacity - buffer.event->size < sizeof (LV2_Event) + size)
            return false;

        LV2_Event* ev = (LV2_Event*) (buffer.event->data + buffer.event->size);
        ev->frames = static_cast<uint32> (frames);
        ev->subframes = 0;
        ev->type = type;
        ev->size = size;
        memcpy ((uint8*) ev + sizeof (LV2_Event), data, size);

        buffer.event->size += portBufferPadSize (sizeof (LV2_Event) + size);
        buffer.event->event_count += 1;
        return true;
    }

    return false;
}

void PortBuffer::clear()
{
    if (isAudio() || isControl())
    {
    }
    else if (isSequence())
    {
        buffer.atom->size = sizeof (LV2_Atom_Sequence_Body);
    }
    else if (isEvent())
    {
        buffer.event->event_count = 0;
        buffer.event->size = 0;
    }
}

void PortBuffer::reset()
{
    if (isAudio() || isCV())
    {
        buffer.atom->size = capacity - sizeof (LV2_Atom);
        buffer.atom->type = bufferType;
    }
    else if (isControl())
    {
        buffer.atom->size = sizeof (float);
        buffer.atom->type = bufferType;
    }
    else if (isSequence())
    {
        buffer.atom->size = input ? sizeof (LV2_Atom_Sequence_Body)
                                  : capacity - sizeof (LV2_Atom_Sequence_Body);
        buffer.atom->type = bufferType;
        LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*) buffer.atom;
        seq->body.unit = 0;
        seq->body.pad = 0;
    }
    else if (isEvent())
    {
        buffer.atom->type = bufferType;
        buffer.event->capacity = capacity - sizeof (LV2_Event_Buffer);
        buffer.event->header_size = sizeof (LV2_Event_Buffer);
        buffer.event->stamp_type = LV2_EVENT_AUDIO_STAMP;
        buffer.event->event_count = 0;
        buffer.event->size = 0;
        buffer.event->data = data.get() + sizeof (LV2_Event_Buffer);
    }
}

void* PortBuffer::getPortData() const
{
    return referenced ? buffer.referred : data.get();
}

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

} // namespace element
