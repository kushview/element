// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

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
