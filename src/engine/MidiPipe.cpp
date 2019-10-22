/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include "engine/MidiPipe.h"

namespace Element {

MidiPipe::MidiPipe()
{ 
    size = 0;
    memset(referencedBuffers, 0, sizeof(MidiBuffer*) * maxReferencedBuffers);
}

MidiPipe::MidiPipe (MidiBuffer** buffers, int numBuffers)
{ 
    jassert (numBuffers < maxReferencedBuffers);
    memset (referencedBuffers, 0, sizeof(MidiBuffer*) * maxReferencedBuffers);
    for (int i = 0; i < numBuffers; ++i)
        referencedBuffers[i] = buffers[i];
    size = numBuffers;
}

MidiPipe::MidiPipe (const OwnedArray<MidiBuffer>& buffers, const Array<int>& channels)
{
    jassert(channels.size() < maxReferencedBuffers);
    memset(referencedBuffers, 0, sizeof(MidiBuffer*) * maxReferencedBuffers);
    for (int i = 0; i < channels.size(); ++i)
        referencedBuffers[i] = buffers.getUnchecked (channels.getUnchecked (i));
    size = channels.size();
}

MidiPipe::~MidiPipe() { }

const MidiBuffer* const MidiPipe::getReadBuffer (const int index) const
{
    jassert (isPositiveAndBelow (index, size));
    return referencedBuffers [index];
}

MidiBuffer* const MidiPipe::getWriteBuffer (const int index) const
{
    jassert (isPositiveAndBelow (index, size));
    return referencedBuffers [index];
}

void MidiPipe::clear()
{
    for (int i = 0; i < maxReferencedBuffers; ++i)
    {
        if (auto* rbuffer = referencedBuffers [i])
            rbuffer->clear();
        else
            break;
    }
}

void MidiPipe::clear (int startSample, int numSamples)
{
    for (int i = 0; i < maxReferencedBuffers; ++i)
    {
        if (auto* rbuffer = referencedBuffers [i])
            rbuffer->clear (startSample, numSamples);
        else
            break;
    }
}

void MidiPipe::clear (int channel, int startSample, int numSamples)
{
    jassert (isPositiveAndBelow (channel, size));
    if (auto* buffer = getWriteBuffer (channel))
        buffer->clear (startSample, numSamples);
}

}
