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

#include "engine/nodes/BaseProcessor.h"
#include "ElementApp.h"

namespace Element {

    enum EqShape
    {
        bell,
        notch,
        highShelf,
        lowShelf,
        highPass,
        lowPass,
    };

    /* Filter for a single EQ band */
    class EQFilter
    {
    public:
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

        void setEqShape (EqShape newShape)
        {
            if (eqShape == newShape)
                return;

            eqShape = newShape;

            switch (eqShape) // Set calcCoefs lambda to correct function for this shape
            {
            case bell:
                calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsBell (fc, Q, gain); };
                break;

            case notch:
                calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsNotch (fc, Q, gain); };
                break;

            case lowShelf:
                calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsLowShelf (fc, Q, gain); };
                break;

            case highShelf:
                calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsHighShelf (fc, Q, gain); };
                break;

            case lowPass:
                calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsLowPass (fc, Q, gain); };
                break;

            case highPass:
                calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsHighPass (fc, Q, gain); };
                break;

            default:
                return;
            }

            calcCoefs (freq.skip (smoothSteps), Q.skip (smoothSteps), gain.skip (smoothSteps));
        }

        /* Calculate filter coefficients for an EQ band (see "Audio EQ Cookbook") */
        void calcCoefsBell (float newFreq, float newQ, float newGain)
        {
            float wc = MathConstants<float>::twoPi * newFreq / fs;
            float c = 1.0f / dsp::FastMathApproximations::tan (wc / 2.0f);
            float phi = c*c;
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

            b[0] = 1.0f / a0;
            b[1] = -2.0f * wC / a0;
            b[2] = 1.0f / a0;

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

            float a0 = ((A+1.0f) + ((A-1.0f) * wC) + (beta*wS));

            b[0] = A*((A+1.0f) - ((A-1.0f)*wC) + (beta*wS)) / a0;
            b[1] = 2.0f*A * ((A-1.0f) - ((A+1.0f)*wC)) / a0;
            b[2] = A*((A+1.0f) - ((A-1.0f)*wC) - (beta*wS)) / a0;

            a[1] = -2.0f * ((A-1.0f) + ((A+1.0f)*wC)) / a0;
            a[2] = ((A+1.0f) + ((A-1.0f)*wC)-(beta*wS)) / a0;
        }

        void calcCoefsHighShelf (float newFreq, float newQ, float newGain)
        {
            float A = sqrtf (newGain);
            float wc = MathConstants<float>::twoPi * newFreq / fs;
            float wS = dsp::FastMathApproximations::sin (wc);
            float wC = dsp::FastMathApproximations::cos (wc);
            float beta = sqrtf (A) / newQ;

            float a0 = ((A+1.0f) - ((A-1.0f) * wC) + (beta*wS));

            b[0] = A*((A+1.0f) + ((A-1.0f)*wC) + (beta*wS)) / a0;
            b[1] = -2.0f*A * ((A-1.0f) + ((A+1.0f)*wC)) / a0;
            b[2] = A*((A+1.0f) + ((A-1.0f)*wC) - (beta*wS)) / a0;

            a[1] = 2.0f * ((A-1.0f) - ((A+1.0f)*wC)) / a0;
            a[2] = ((A+1.0f) - ((A-1.0f)*wC)-(beta*wS)) / a0;
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

            z[1] = z[2] + x*b[1] - y*a[1];
            z[2] = x*b[2] - y*a[2];

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
            calcCoefs (freq.skip (smoothSteps), Q.skip (smoothSteps), gain.skip (smoothSteps));
        }

    private:
        SmoothedValue<float, ValueSmoothingTypes::Linear> freq;
        SmoothedValue<float, ValueSmoothingTypes::Linear> Q;
        SmoothedValue<float, ValueSmoothingTypes::Linear> gain;
        const int smoothSteps = 500;

        EqShape eqShape = bell;
        typedef std::function<void (float, float, float)> CalcCoefsLambda;
        CalcCoefsLambda calcCoefs = [this] (float fc, float Q, float gain) { calcCoefsBell (fc, Q, gain); }; // lambda function to calculate coefficients for any shape

        float b[3] = { 1.0f, 0.0f, 0.0f };
        float a[3] = { 1.0f, 0.0f, 0.0f };
        float z[3] = { 0.0f, 0.0f, 0.0f };

        float fs = 44100.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQFilter)
    };

    class EQFilterProcessor : public BaseProcessor
    {
    private:
        const bool stereo;
        AudioParameterFloat* freq     = nullptr;
        AudioParameterFloat* q        = nullptr;
        AudioParameterFloat* gainDB   = nullptr;
        AudioParameterChoice* eqShape = nullptr;

    public:
        explicit EQFilterProcessor (const bool _stereo = false)
            : BaseProcessor(),
            stereo (_stereo)
        {
            setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1, 44100.0, 1024);

            NormalisableRange<float> freqRange (20.0f, 22000.0f);
            freqRange.setSkewForCentre (1000.0f);

            NormalisableRange<float> qRange (0.1f, 18.0f);
            qRange.setSkewForCentre (0.707f);

            addParameter (freq   = new AudioParameterFloat ("freq", "Cutoff Frequency [Hz]", freqRange, 1000.0f));
            addParameter (q      = new AudioParameterFloat ("q",    "Filter Q",              qRange,    0.707f));
            addParameter (gainDB = new AudioParameterFloat ("gain", "Filter Gain [dB]",      -15.0f, 15.0f, 0.0f));
            
            addParameter (eqShape = new AudioParameterChoice ("shape", "EQ Shape", {"Bell", "Notch", "Hi Shelf", "Low Shelf", "HPF", "LPF"}, 0));
        }

        const String getName() const override { return "EQ Filter"; }

        void fillInPluginDescription (PluginDescription& desc) const override
        {
            desc.name = getName();
            desc.fileOrIdentifier   = stereo ? "element.eqfilt.stereo" : "element.eqfilt.mono";
            desc.descriptiveName    = stereo ? "EQ Filter (stereo)" : "EQ Filter (mono)";
            desc.numInputChannels   = stereo ? 2 : 1;
            desc.numOutputChannels  = stereo ? 2 : 1;
            desc.hasSharedContainer = false;
            desc.isInstrument       = false;
            desc.manufacturerName   = "Element";
            desc.pluginFormatName   = "Element";
            desc.version            = "1.0.0";
        }

        void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                eqFilter[ch].setFrequency (*freq);
                eqFilter[ch].setQ (*q);
                eqFilter[ch].setGain (Decibels::decibelsToGain ((float) *gainDB));
                eqFilter[ch].setEqShape ((EqShape) eqShape->getIndex());

                eqFilter[ch].reset (sampleRate);
            }

            setPlayConfigDetails (stereo ? 2 : 1, stereo ? 2 : 1,
                sampleRate, maximumExpectedSamplesPerBlock);
        }

        void releaseResources() override
        {
        }

        void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
        {
            const int numChans = jmin (2, buffer.getNumChannels());
            auto** output = buffer.getArrayOfWritePointers();
            for (int c = 0; c < numChans; ++c)
            {
                eqFilter[c].setFrequency (*freq);
                eqFilter[c].setQ (*q);
                eqFilter[c].setGain (Decibels::decibelsToGain ((float) *gainDB));
                eqFilter[c].setEqShape ((EqShape) eqShape->getIndex());

                eqFilter[c].processBlock (output[c], buffer.getNumSamples());
            }
        }

        AudioProcessorEditor* createEditor() override   { return new GenericAudioProcessorEditor (this); }
        bool hasEditor() const override                 { return true; }

        double getTailLengthSeconds() const override    { return 0.0; };
        bool acceptsMidi() const override               { return false; }
        bool producesMidi() const override              { return false; }

        int getNumPrograms() override                                      { return 1; };
        int getCurrentProgram() override                                   { return 1; };
        void setCurrentProgram (int index) override                        { ignoreUnused (index); };
        const String getProgramName (int index) override                   { ignoreUnused (index); return "Parameter"; }
        void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

        void getStateInformation (juce::MemoryBlock& destData) override
        {
            ValueTree state (Tags::state);
            state.setProperty ("freq",   (float) *freq,   0);
            state.setProperty ("q",      (float) *q,      0);
            state.setProperty ("gainDB", (float) *gainDB, 0);
            state.setProperty ("shape",  (int) eqShape->getIndex(), 0);
            if (auto e = state.createXml())
                AudioProcessor::copyXmlToBinary (*e, destData);
        }

        void setStateInformation (const void* data, int sizeInBytes) override
        {
            if (auto e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
            {
                auto state = ValueTree::fromXml (*e);
                if (state.isValid())
                {
                    *freq    = (float) state.getProperty ("freq",   (float) *freq);
                    *q       = (float) state.getProperty ("q",      (float) *q);
                    *gainDB  = (float) state.getProperty ("gainDB", (float) *gainDB);
                    *eqShape = (int)   state.getProperty ("shape",  (int)   *eqShape);
                }
            }
        }

    private:
        EQFilter eqFilter[2];
    };

}
