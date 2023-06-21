// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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

    int getNumProcessors() const { return processors.size(); }
    ProcessorType* getProcessor (int index) const { return processors[index]; }

    float getLatencySamples (int index) const;
    int getFactor (int index) const;
    void prepare (int numChannels, int blockSize);
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
