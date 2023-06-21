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

#include "ringbuffer.hpp"

namespace element {

RingBuffer::RingBuffer (int32 capacity)
    : fifo (1), buffer (nullptr)
{
    setCapacity (capacity);
}

RingBuffer::~RingBuffer()
{
    fifo.reset();
    fifo.setTotalSize (1);
    buffer = nullptr;
    block.free();
}

void RingBuffer::setCapacity (int32 newCapacity)
{
    newCapacity = juce::nextPowerOfTwo (newCapacity);

    if (fifo.getTotalSize() != newCapacity)
    {
        juce::HeapBlock<uint8> newBlock;
        newBlock.allocate (newCapacity, true);
        {
            block.swapWith (newBlock);
            buffer = block.getData();
            fifo.setTotalSize (newCapacity);
        }
    }

    fifo.reset();
}

} // namespace element
