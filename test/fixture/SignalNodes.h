// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cmath>

#include "TestNode.h"

namespace element {
namespace test {

/** Deterministic signal so different render paths see identical input. */
inline float inputSample (int ch, int i)
{
    return 0.5f * std::sin ((float) (i + ch * 37) * 0.013f)
           + 0.25f * std::cos ((float) (i * 3 + ch) * 0.007f);
}

inline void fillDeterministic (juce::AudioSampleBuffer& buf)
{
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample (ch, i, inputSample (ch, i));
}

/** Generator: writes a deterministic per-channel signal to its outputs. */
class GenNode : public TestNode {
public:
    explicit GenNode (int numOuts, float amp = 1.0f)
        : TestNode (0, numOuts, 0, 0), amplitude (amp) {}

    void render (RenderContext& rc) override
    {
        const int numSamples = rc.audio.getNumSamples();
        const int numChans = juce::jmin (getNumAudioOutputs(), rc.audio.getNumChannels());
        for (int ch = 0; ch < numChans; ++ch) {
            auto* out = rc.audio.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                out[i] = amplitude * (0.3f * std::sin ((float) (i + ch * 11) * 0.02f) + 0.2f);
        }
        ++renderCount;
    }

    /** Number of render() calls; proves the node kept processing. */
    std::atomic<int> renderCount { 0 };

private:
    float amplitude;
};

/** In-place gain: out[ch] = in[ch] * gain for each shared channel. */
class GainNode : public TestNode {
public:
    GainNode (int numChans, float g)
        : TestNode (numChans, numChans, 0, 0), gain (g) {}

    void render (RenderContext& rc) override
    {
        const int numSamples = rc.audio.getNumSamples();
        const int numChans = juce::jmin (getNumAudioInputs(), rc.audio.getNumChannels());
        for (int ch = 0; ch < numChans; ++ch) {
            auto* d = rc.audio.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                d[i] *= gain;
        }
    }

private:
    float gain;
};

} // namespace test
} // namespace element
