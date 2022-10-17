/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

#pragma once

#include "JuceHeader.h"

namespace element {

class RingBuffer
{
public:
    RingBuffer (int32 capacity);
    ~RingBuffer();

    void setCapacity (int32 newCapacity);
    inline size_t size() const { return (size_t) fifo.getTotalSize(); }

    inline bool canRead (uint32 bytes) const { return bytes <= (uint32) fifo.getNumReady() && bytes != 0; }
    inline uint32 getReadSpace() const { return fifo.getNumReady(); }

    inline bool canWrite (uint32 bytes) const { return bytes <= (uint32) fifo.getFreeSpace() && bytes != 0; }
    inline uint32 getWriteSpace() const { return (uint32) fifo.getFreeSpace(); }

    inline uint32
        peak (void* dest, uint32 size)
    {
        return read (dest, size, false);
    }

    inline void advance (uint32 bytes, bool write)
    {
        if (write)
            fifo.finishedWrite (static_cast<int> (bytes));
        else
            fifo.finishedRead (static_cast<int> (bytes));
    }

    inline uint32
        read (void* dest, uint32 size, bool advance = true)
    {
        buffer = block.getData();
        fifo.prepareToRead (size, vec1.index, vec1.size, vec2.index, vec2.size);

        if (vec1.size > 0)
            memcpy (dest, buffer + vec1.index, vec1.size);

        if (vec2.size > 0)
            memcpy ((uint8*) dest + vec1.size, buffer + vec2.index, vec2.size);

        if (advance)
            fifo.finishedRead (vec1.size + vec2.size);

        return vec1.size + vec2.size;
    }

    template <typename T>
    inline uint32 read (T& dest, bool advance = true)
    {
        return read (&dest, sizeof (T), advance);
    }

    inline void advanceReadPointer (const uint32)
    {
        jassertfalse;
    }

    inline uint32
        write (const void* src, uint32 bytes)
    {
        buffer = block.getData();
        fifo.prepareToWrite (bytes, vec1.index, vec1.size, vec2.index, vec2.size);

        if (vec1.size > 0)
            memcpy (buffer + vec1.index, src, vec1.size);

        if (vec2.size > 0)
            memcpy (buffer + vec2.index, (uint8*) src + vec1.size, vec2.size);

        fifo.finishedWrite (vec1.size + vec2.size);
        return vec1.size + vec2.size;
    }

    template <typename T>
    inline uint32 write (const T& src)
    {
        return write (&src, sizeof (T));
    }

    struct Vector
    {
        uint32 size;
        void* buffer;
    };

private:
    struct Vec
    {
        int32 size;
        int32 index;
    };

    Vec vec1, vec2;
    juce::AbstractFifo fifo;
    juce::HeapBlock<uint8> block;
    uint8* buffer;
};

} // namespace element
