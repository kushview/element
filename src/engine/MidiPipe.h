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

#pragma once

#include "JuceHeader.h"

namespace Element {

/** A glorified array of MidiBuffers used in rendering graph nodes */
class MidiPipe
{
public:
    MidiPipe();
    MidiPipe (MidiBuffer** buffers, int numBuffers);
    MidiPipe (const OwnedArray<MidiBuffer>& buffers, const Array<int>& channels);
    ~MidiPipe();

    int getNumBuffers() const { return size; }
    const MidiBuffer* const getReadBuffer (const int index) const;
    MidiBuffer* const getWriteBuffer (const int index) const;

    void clear();
    void clear (int startSample, int numSamples);
    void clear (int index, int startSample, int numSamples);

private:
    enum { maxReferencedBuffers = 32 };
    int size = 0;
    MidiBuffer* referencedBuffers [maxReferencedBuffers];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPipe);
};

}
