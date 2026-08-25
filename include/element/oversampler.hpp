// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/core.hpp>
#include <element/juce/dsp.hpp>

namespace element {

template <typename SampleType>
class Oversampler final {
public:
    using ProcessorType = juce::dsp::Oversampling<SampleType>;

    Oversampler() = default;
    ~Oversampler();

    int getNumProcessors() const noexcept { return processors.size(); }
    ProcessorType* getProcessor (int index) const { return processors[index]; }

    float getLatencySamples (int index) const;
    int getFactor (int index) const;
    void prepare (int numChannels, int blockSize);

    /** Returns the oversampling chain for the given index, creating and
        initializing it on demand with the spec given to prepare().

        Chains are expensive to build (IIR filter design + buffers), so they
        are only created here, never in prepare(). Must be called on the
        message thread, and only after prepare() has set a valid spec.

        @param index  the processor index; the oversample factor is 2^(index + 1)
        @return the chain, or nullptr if the index or spec is invalid
    */
    ProcessorType* ensureProcessor (int index);

    void reset();

private:
    enum {
        maxProc = 3
    };
    int channels = 0,
        buffer = 0;
    juce::OwnedArray<ProcessorType> processors;
};

} // namespace element
