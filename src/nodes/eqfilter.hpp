// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nodes/baseprocessor.hpp"
#include "ElementApp.h"

namespace element {

/* Filter for a single EQ band */
class EQFilter
{
public:
    enum Shape
    {
        Bell,
        Notch,
        HighShelf,
        LowShelf,
        HighPass,
        LowPass,
    };

    EQFilter()
    {
        freq.reset (smoothSteps);
        Q.reset (smoothSteps);
        gain.reset (smoothSteps);
    }

    void setFrequency (float newFreq)
    {
        if (newFreq != freq.getTargetValue())
            // don't allow the cutoff frequency to get to close to Nyquist (could go unstable)
            freq.setTargetValue (jmin (newFreq, fs / 2.0f - 100.0f));
    }

    void setQ (float newQ)
    {
        if (newQ != Q.getTargetValue())
            Q.setTargetValue (newQ);
    }

    void setGain (float newGain)
    {
        if (newGain != gain.getTargetValue())
            gain.setTargetValue (newGain);
    }

    void setShape (Shape newShape)
    {
        if (eqShape == newShape)
            return;

        // Finish an existing transition first; the processor retries the latest
        // requested shape next block. This also handles rapid knob changes.
        if (shapeFadeRemaining > 0)
            return;
        std::copy (std::begin (a), std::end (a), oldA);
        std::copy (std::begin (b), std::end (b), oldB);
        std::copy (std::begin (z), std::end (z), oldZ);
        shapeFadeRemaining = shapeFadeLength;
        eqShape = newShape;

        calcCoefs (freq.skip (smoothSteps), Q.skip (smoothSteps), gain.skip (smoothSteps));
    }

    /* Calculate filter coefficients for an EQ band (see "Audio EQ Cookbook") */
    void calcCoefsBell (float newFreq, float newQ, float newGain)
    {
        float wc = MathConstants<float>::twoPi * newFreq / fs;
        float c = 1.0f / dsp::FastMathApproximations::tan (wc / 2.0f);
        float phi = c * c;
        float Knum = c / newQ;
        float Kdenom = Knum;

        if (newGain > 1.0f)
            Knum *= newGain;
        else if (newGain < 1.0f)
            Kdenom /= newGain;

        float a0 = phi + Kdenom + 1.0f;

        b[0] = (phi + Knum + 1.0f) / a0;
        b[1] = 2.0f * (1.0f - phi) / a0;
        b[2] = (phi - Knum + 1.0f) / a0;

        a[1] = 2.0f * (1.0f - phi) / a0;
        a[2] = (phi - Kdenom + 1.0f) / a0;
    }

    void calcCoefsNotch (float newFreq, float newQ, float newGain)
    {
        float wc = MathConstants<float>::twoPi * newFreq / fs;
        float wS = dsp::FastMathApproximations::sin (wc);
        float wC = dsp::FastMathApproximations::cos (wc);
        float alpha = wS / (2.0f * newQ);

        float a0 = 1.0f + alpha;

        b[0] = newGain / a0;
        b[1] = -2.0f * wC * b[0];
        b[2] = b[0];

        a[1] = -2.0f * wC / a0;
        a[2] = (1.0f - alpha) / a0;
    }

    void calcCoefsLowShelf (float newFreq, float newQ, float newGain)
    {
        float A = sqrtf (newGain);
        float wc = MathConstants<float>::twoPi * newFreq / fs;
        float wS = dsp::FastMathApproximations::sin (wc);
        float wC = dsp::FastMathApproximations::cos (wc);
        float beta = sqrtf (A) / newQ;

        float a0 = ((A + 1.0f) + ((A - 1.0f) * wC) + (beta * wS));

        b[0] = A * ((A + 1.0f) - ((A - 1.0f) * wC) + (beta * wS)) / a0;
        b[1] = 2.0f * A * ((A - 1.0f) - ((A + 1.0f) * wC)) / a0;
        b[2] = A * ((A + 1.0f) - ((A - 1.0f) * wC) - (beta * wS)) / a0;

        a[1] = -2.0f * ((A - 1.0f) + ((A + 1.0f) * wC)) / a0;
        a[2] = ((A + 1.0f) + ((A - 1.0f) * wC) - (beta * wS)) / a0;
    }

    void calcCoefsHighShelf (float newFreq, float newQ, float newGain)
    {
        float A = sqrtf (newGain);
        float wc = MathConstants<float>::twoPi * newFreq / fs;
        float wS = dsp::FastMathApproximations::sin (wc);
        float wC = dsp::FastMathApproximations::cos (wc);
        float beta = sqrtf (A) / newQ;

        float a0 = ((A + 1.0f) - ((A - 1.0f) * wC) + (beta * wS));

        b[0] = A * ((A + 1.0f) + ((A - 1.0f) * wC) + (beta * wS)) / a0;
        b[1] = -2.0f * A * ((A - 1.0f) + ((A + 1.0f) * wC)) / a0;
        b[2] = A * ((A + 1.0f) + ((A - 1.0f) * wC) - (beta * wS)) / a0;

        a[1] = 2.0f * ((A - 1.0f) - ((A + 1.0f) * wC)) / a0;
        a[2] = ((A + 1.0f) - ((A - 1.0f) * wC) - (beta * wS)) / a0;
    }

    void calcCoefsLowPass (float newFreq, float newQ, float newGain)
    {
        float wc = MathConstants<float>::twoPi * newFreq / fs;
        float c = 1.0f / dsp::FastMathApproximations::tan (wc / 2.0f);
        float phi = c * c;
        float K = c / newQ;
        float a0 = phi + K + 1.0f;

        b[0] = newGain / a0;
        b[1] = 2.0f * b[0];
        b[2] = b[0];
        a[1] = 2.0f * (1.0f - phi) / a0;
        a[2] = (phi - K + 1.0f) / a0;
    }

    void calcCoefsHighPass (float newFreq, float newQ, float newGain)
    {
        float wc = MathConstants<float>::twoPi * newFreq / fs;
        float c = 1.0f / dsp::FastMathApproximations::tan (wc / 2.0f);
        float phi = c * c;
        float K = c / newQ;
        float a0 = phi + K + 1.0f;

        b[0] = newGain * phi / a0;
        b[1] = -2.0f * b[0];
        b[2] = b[0];
        a[1] = 2.0f * (1.0f - phi) / a0;
        a[2] = (phi - K + 1.0f) / a0;
    }

    inline float process (float x)
    {
        // process input sample, direct form II transposed
        float y = z[1] + x * b[0];

        z[1] = z[2] + x * b[1] - y * a[1];
        z[2] = x * b[2] - y * a[2];

        if (shapeFadeRemaining > 0)
        {
            const float previous = oldZ[1] + x * oldB[0];
            oldZ[1] = oldZ[2] + x * oldB[1] - previous * oldA[1];
            oldZ[2] = x * oldB[2] - previous * oldA[2];
            const float mix = float (shapeFadeRemaining--) / float (shapeFadeLength);
            return previous * mix + y * (1.0f - mix);
        }
        return y;
    }

    void processBlock (float* buffer, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            if (freq.isSmoothing() || Q.isSmoothing() || gain.isSmoothing())
                calcCoefs (freq.getNextValue(), Q.getNextValue(), gain.getNextValue());
            buffer[n] = process (buffer[n]);
        }
    }

    void reset (double sampleRate)
    {
        // clear state
        for (int n = 0; n < 3; ++n)
            z[n] = 0.0f;

        fs = (float) sampleRate;
        smoothSteps = jmax (1, roundToInt (sampleRate * 0.01));
        shapeFadeLength = smoothSteps;
        shapeFadeRemaining = 0;
        const float f = jlimit (1.0f, jmax (1.0f, fs * 0.5f - 100.0f), freq.getTargetValue());
        const float q = jmax (0.1f, Q.getTargetValue());
        const float g = jmax (0.000001f, gain.getTargetValue());
        freq.reset (smoothSteps);
        Q.reset (smoothSteps);
        gain.reset (smoothSteps);
        freq.setCurrentAndTargetValue (f);
        Q.setCurrentAndTargetValue (q);
        gain.setCurrentAndTargetValue (g);
        calcCoefs (f, q, g);
    }

    /** Get the magnitude of the filter at this frequency, in units of linear gain */
    float getMagnitudeAtFreq (float f)
    {
        std::complex<float> s (0.0f, f / freq.getTargetValue()); // s = j (w  / w0)
        std::complex<float> numerator (1.0f, 0.0f);
        std::complex<float> denominator (1.0f, 0.0f);

        if (eqShape == Bell)
        {
            //         s^2 + s * (A / Q) + 1
            // H(s) = -----------------------
            //         s^2 + s / (A * Q) + 1

            auto A = powf (10.0f, Decibels::gainToDecibels (gain.getTargetValue()) / 40.0f);
            numerator = s * s + s * A / Q.getTargetValue() + 1.0f;
            denominator = s * s + s / (A * Q.getTargetValue()) + 1.0f;
        }
        else if (eqShape == Notch)
        {
            //             s^2 + 1
            // H(s) = -----------------
            //         s^2 + s / Q + 1

            numerator = s * s + 1.0f;
            denominator = s * s + s / Q.getTargetValue() + 1.0f;
            numerator *= gain.getTargetValue();
        }
        else if (eqShape == LowShelf)
        {
            //           s^2 + s * (sqrt(A) / Q) + A
            // H(s) = ---------------------------------
            //         A * s^2 + s * (sqrt(A) / Q) + 1

            auto A = powf (10.0f, Decibels::gainToDecibels (gain.getTargetValue()) / 40.0f);
            numerator = s * s + s * sqrtf (A) / Q.getTargetValue() + A;
            denominator = A * s * s + s * sqrtf (A) / Q.getTargetValue() + 1.0f;
            numerator *= A;
        }
        else if (eqShape == HighShelf)
        {
            //         A * s^2 + s * (sqrt(A) / Q) + 1
            // H(s) = ---------------------------------
            //           s^2 + s * (sqrt(A) / Q) + A

            auto A = powf (10.0f, Decibels::gainToDecibels (gain.getTargetValue()) / 40.0f);
            numerator = A * s * s + s * sqrtf (A) / Q.getTargetValue() + 1.0f;
            denominator = s * s + s * sqrtf (A) / Q.getTargetValue() + A;
            numerator *= A;
        }
        else if (eqShape == LowPass)
        {
            //               1
            // H(s) = -----------------
            //         s^2 + s / Q + 1

            numerator = 1.0f;
            denominator = s * s + s / Q.getTargetValue() + 1.0f;
            numerator *= gain.getTargetValue();
        }
        else if (eqShape == HighPass)
        {
            //              s^2
            // H(s) = -----------------
            //         s^2 + s / Q + 1

            numerator = s * s;
            denominator = s * s + s / Q.getTargetValue() + 1.0f;
            numerator *= gain.getTargetValue();
        }

        return abs (numerator / denominator); // |H(s)|
    }

private:
    SmoothedValue<float, ValueSmoothingTypes::Linear> freq;
    SmoothedValue<float, ValueSmoothingTypes::Linear> Q;
    SmoothedValue<float, ValueSmoothingTypes::Linear> gain;
    int smoothSteps = 441;
    int shapeFadeLength = 441, shapeFadeRemaining = 0;
    float oldA[3] {}, oldB[3] {}, oldZ[3] {};

    Shape eqShape = Bell;
    void calcCoefs (float fc, float q, float g)
    {
        switch (eqShape)
        {
            case Bell:
                calcCoefsBell (fc, q, g);
                break;
            case Notch:
                calcCoefsNotch (fc, q, g);
                break;
            case LowShelf:
                calcCoefsLowShelf (fc, q, g);
                break;
            case HighShelf:
                calcCoefsHighShelf (fc, q, g);
                break;
            case LowPass:
                calcCoefsLowPass (fc, q, g);
                break;
            case HighPass:
                calcCoefsHighPass (fc, q, g);
                break;
        }
    }

    float b[3] = { 1.0f, 0.0f, 0.0f };
    float a[3] = { 1.0f, 0.0f, 0.0f };
    float z[3] = { 0.0f, 0.0f, 0.0f };

    float fs = 44100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQFilter)
};

class EQFilterProcessor : public BaseProcessor
{
public:
    explicit EQFilterProcessor (const int _numChannels = 2);

    const String getName() const override { return "EQ Filter"; }

    void fillInPluginDescription (PluginDescription& desc) const override;

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override;

    void updateParams();
    float getMagnitudeAtFreq (float freq) { return eqFilter[0].getMagnitudeAtFreq (freq); }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; };
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 1; };
    void setCurrentProgram (int index) override { ignoreUnused (index); };
    const String getProgramName (int index) override
    {
        ignoreUnused (index);
        return "Parameter";
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void numChannelsChanged() override;

protected:
    inline bool isBusesLayoutSupported (const BusesLayout& layout) const override
    {
        // supports single bus only
        if (layout.inputBuses.size() != 1 && layout.outputBuses.size() != 1)
            return false;

        // ins must equal outs
        if (layout.getMainInputChannels() != layout.getMainOutputChannels())
            return false;

        const auto nchans = layout.getMainInputChannels();
        return nchans >= 1 && nchans <= 2;
    }

    inline bool canApplyBusesLayout (const BusesLayout& layouts) const override { return isBusesLayoutSupported (layouts); }
    inline bool canApplyBusCountChange (bool isInput, bool isAddingBuses, BusProperties& outNewBusProperties) override
    {
        ignoreUnused (isInput, isAddingBuses, outNewBusProperties);
        return false;
    }

private:
    int numChannels = 0;
    AudioParameterFloat* freq = nullptr;
    AudioParameterFloat* q = nullptr;
    AudioParameterFloat* gainDB = nullptr;
    AudioParameterChoice* eqShape = nullptr;
    EQFilter eqFilter[2];
};

} // namespace element
