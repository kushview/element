// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <cassert>
#include <element/element.hpp>
#include <element/juce/core.hpp>

namespace element {

/** A glorified array of Buffers used in rendering graph nodes */
template <class Buf>
class DataPipe {
public:
    using buffer_type = Buf;

    DataPipe()
        : _size (0)
    {
        memset (referencedBuffers, 0, sizeof (Buf*) * maxReferencedBuffers);
    }

    DataPipe (Buf& buffer)
    {
        memset (referencedBuffers, 0, sizeof (Buf*) * maxReferencedBuffers);
        referencedBuffers[0] = &buffer;
        _size = 1;
    }

    DataPipe (Buf** buffers, int numBuffers)
    {
        assert (numBuffers < maxReferencedBuffers);
        memset (referencedBuffers, 0, sizeof (Buf*) * maxReferencedBuffers);
        for (int i = 0; i < numBuffers; ++i)
            referencedBuffers[i] = buffers[i];
        _size = numBuffers;
    }

    DataPipe (const juce::OwnedArray<Buf>& buffers, const juce::Array<int>& channels)
    {
        jassert (channels.size() < maxReferencedBuffers);
        memset (referencedBuffers, 0, sizeof (Buf*) * maxReferencedBuffers);
        for (int i = 0; i < channels.size(); ++i)
            referencedBuffers[i] = buffers.getUnchecked (channels.getUnchecked (i));
        _size = channels.size();
    }

    ~DataPipe()
    {
    }

    inline constexpr int size() const noexcept { return _size; }

    inline const auto* const readBuffer (const int index) const noexcept
    {
        assert (index >= 0 && index < _size);
        return referencedBuffers[index];
    }

    inline auto* const writeBuffer (const int index) const noexcept
    {
        assert (index >= 0 && index < _size);
        return referencedBuffers[index];
    }

    inline void clear()
    {
        for (int i = _size; --i >= 0;)
            referencedBuffers[i]->clear();
    }

    inline void clear (int startSample, int numSamples)
    {
        for (int i = _size; --i >= 0;)
            referencedBuffers[i]->clear (startSample, numSamples);
    }

    inline void clear (int index, int startSample, int numSamples)
    {
        assert (index >= 0 && index < _size);
        referencedBuffers[index]->clear (startSample, numSamples);
    }

private:
    enum { maxReferencedBuffers = 32 };
    int _size = 0;
    buffer_type* referencedBuffers[maxReferencedBuffers];
    EL_DISABLE_COPY (DataPipe)
};

} // namespace element
