/*
    This file is part of Element
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

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

template <typename SampleType>
class Oversampler final
{
public:
    using ProcessorType = juce::dsp::Oversampling<SampleType>;

    Oversampler() = default;
    ~Oversampler();

    int getNumProcessors() const { return processors.size(); }
    ProcessorType* getProcessor (int index) const { return processors[index]; }

    float getLatencySamples (int index) const;
    int getFactor (int index) const;
    void prepare (int numChannels, int blockSize);
    void reset();

private:
    enum
    {
        maxProc = 3
    };
    int channels = 0,
        buffer = 0;
    OwnedArray<ProcessorType> processors;
};

} // namespace element
