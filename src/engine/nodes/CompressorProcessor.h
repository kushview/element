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

/**
    A simple level detector with exponential attack characteristic,
    and exponential, return-to-zero release characteristic.
*/
class LevelDetector
{
public:
    LevelDetector() {}

    void setAttackMs  (float newAttackMs)
    {
        if (attackMs != newAttackMs)
        {
            attackMs = newAttackMs;
            a1_a = expf (-1.0f / (fs * attackMs / 1000.0f));
            b0_a = 1.0f - a1_a;
        }
    }

    void setReleaseMs (float newReleaseMs)
    {
        if (releaseMs != newReleaseMs)
        {
            releaseMs = newReleaseMs;
            a1_r = expf (-1.0f / (fs * releaseMs / 1000.0f));
            b0_r = 1.0f - a1_r;
        }
    }

    /* Set sample rate, reset levelEstimate */
    void reset (float sampleRate)
    {
        fs = sampleRate;
        levelEstimate = 0.0f;

        setAttackMs (attackMs);
        setReleaseMs (releaseMs);
    }

    /* Process a single sample */
    inline float process (float x)
    {
        if (fabs (x) > levelEstimate)
            levelEstimate += b0_a * (fabs (x) - levelEstimate);
        else
            levelEstimate += b0_r * (fabs (x) - levelEstimate);

        return levelEstimate;
    }

    void setLevelEstimate (float levelEst) { levelEstimate = levelEst; }
    float getLevelEstimate() { return levelEstimate; }

private:
    // Attack coefs
    float attackMs = 1.0f;
    float a1_a = 0.0f;
    float b0_a = 1.0f;

    // Release coefs
    float releaseMs = 50.0f;
    float a1_r = 0.0f;
    float b0_r = 1.0f;

    float levelEstimate = 0.0f;
    float fs = 48000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelDetector)
};

/** Feedforward gain computer for compressor */
class GainComputer
{
public:
    GainComputer()
    {
        thresh.reset (numSteps);
        ratio.reset  (numSteps);
    }

    void setThreshold (float newThreshDB)
    {
        if (threshDB != newThreshDB)
        {
            threshDB = newThreshDB;
            thresh.setTargetValue (Decibels::decibelsToGain (newThreshDB));
            recalcKnees();
        }    
    }
    
    void setRatio (float newRatio)
    {
        if (ratio.getTargetValue() != newRatio)
        {
            ratio.setTargetValue (newRatio);
            recalcAs();
        }
    }

    void setKnee (float newKneeDB)
    {
        if (kneeDB != newKneeDB)
        {
            kneeDB = newKneeDB;
            recalcKnees();
            recalcAs();
        }
    }

    void reset()
    {
        thresh.skip (numSteps);
        ratio.skip (numSteps);
    }

    inline float process (float x)
    {
        auto curThresh = thresh.getNextValue();
        auto curRatio = ratio.getNextValue();

        auto xAbs = fabsf (x);
        if (xAbs <= kneeLower) // below thresh
            return 1.0f;

        if (xAbs >= kneeUpper) // compression range
            return powf (x / curThresh, (1.0f / curRatio) - 1.0f);

        // knee range
        auto gainCorr = Decibels::gainToDecibels (xAbs) - Decibels::gainToDecibels (curThresh) + 0.5f * kneeDB;
        auto gainDB = -1.0f * aFF * gainCorr * gainCorr;
        return Decibels::decibelsToGain (gainDB);
    }

private:
    // recalculate knee values for a new threshold or knee width
    void recalcKnees()
    {
        kneeLower = Decibels::decibelsToGain (threshDB - kneeDB / 2.0f);
        kneeUpper = Decibels::decibelsToGain (threshDB + kneeDB / 2.0f);
    }

    // recalculate A values for new ratio or knee width
    void recalcAs()
    {
        aFF = (1.0f - (1.0f / ratio.getTargetValue())) / (2.0f * kneeDB);
    }

    float threshDB = 0.0f;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> thresh = 1.0f;
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> ratio = 1.0f;
    const int numSteps = 500;

    float kneeDB = 1.0f;

    float kneeUpper = 1.0f;
    float kneeLower = 1.0f;
    float aFF = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainComputer)
};

/** Compressor Processing */
class CompressorProcessor : public BaseProcessor
{
public:
    explicit CompressorProcessor (const int _numChannels = 2)
        : BaseProcessor (BusesProperties()
            .withInput ("Main", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))
            .withOutput ("Main", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))),
        numChannels (jlimit (1, 2, _numChannels))
    {
        setPlayConfigDetails (numChannels, numChannels, 44100.0, 1024);

        NormalisableRange<float> ratioRange (0.5f, 10.0f);
        ratioRange.setSkewForCentre (2.0f);

        NormalisableRange<float> attackRange (0.1f, 1000.0f);
        attackRange.setSkewForCentre (10.0f);

        NormalisableRange<float> releaseRange (10.0f, 3000.0f);
        releaseRange.setSkewForCentre (100.0f);

        addParameter (threshDB  = new AudioParameterFloat ("thresh",  "Threshold [dB]", -30.0f, 0.0f, 0.0f));
        addParameter (ratio     = new AudioParameterFloat ("ratio",   "Ratio",          ratioRange, 1.0f));
        addParameter (kneeDB    = new AudioParameterFloat ("knee",    "Knee [dB]",      0.0f, 12.0f, 6.0f));
        addParameter (attackMs  = new AudioParameterFloat ("attack",  "Attack [ms]",    attackRange, 10.0f));
        addParameter (releaseMs = new AudioParameterFloat ("release", "Release [ms]",   releaseRange, 100.0f));
        addParameter (makeupDB  = new AudioParameterFloat ("makeup",  "Makeup [dB]",    -18.0f, 18.0f, 0.0f));

        makeupGain.reset (numSteps);
    }

    const String getName() const override { return "Compressor"; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = EL_INTERNAL_ID_COMPRESSOR;
        desc.descriptiveName    = "Compressor";
        desc.numInputChannels   = 2;
        desc.numOutputChannels  = 2;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
        desc.uid                = EL_INTERNAL_UID_COMPRESSOR;
    }

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        detector.reset ((float) sampleRate);
        gainComputer.reset();

        setPlayConfigDetails (numChannels, numChannels, sampleRate, maximumExpectedSamplesPerBlock);
    }

    void releaseResources() override
    {
    }

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        auto numBuffChannels = buffer.getNumChannels();
        
        // update params
        detector.setAttackMs (*attackMs);
        detector.setReleaseMs (*releaseMs);
        gainComputer.setThreshold (*threshDB);
        gainComputer.setRatio (*ratio);
        gainComputer.setKnee (*kneeDB);

        makeupGain.setTargetValue (Decibels::decibelsToGain ((float) *makeupDB));

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            // Sum input from channels
            float detectorInput = 0.0f;
            for (int ch = 0; ch < numBuffChannels; ++ch)
                detectorInput += buffer.getSample (ch, n);
            detectorInput /= (float) numBuffChannels;

            // Get level estimate, and compute gain
            float level = detector.process (detectorInput);
            float gain = gainComputer.process (level);

            // Apply gain
            buffer.applyGain (n, 1, gain * makeupGain.getNextValue());
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
        state.setProperty ("thresh",  (float) *threshDB,  0);
        state.setProperty ("ratio",   (float) *ratio,     0);
        state.setProperty ("knee",    (float) *kneeDB,    0);
        state.setProperty ("attack",  (float) *attackMs,  0);
        state.setProperty ("release", (float) *releaseMs, 0);
        state.setProperty ("makeup",  (float) *makeupDB,  0);
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
                *threshDB  = (float) state.getProperty ("thresh",  (float) *threshDB);
                *ratio     = (float) state.getProperty ("ratio",   (float) *ratio);
                *kneeDB    = (float) state.getProperty ("knee",    (float) *kneeDB);
                *attackMs  = (float) state.getProperty ("attack",  (float) *attackMs);
                *releaseMs = (float) state.getProperty ("release", (float) *releaseMs);
                *makeupDB  = (float) state.getProperty ("makeup",  (float) *makeupDB);
            }
        }
    }

    void numChannelsChanged() override
    {
        numChannels = getTotalNumInputChannels();
    }

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
    AudioParameterFloat* threshDB  = nullptr;
    AudioParameterFloat* ratio     = nullptr;
    AudioParameterFloat* kneeDB    = nullptr;
    AudioParameterFloat* attackMs  = nullptr;
    AudioParameterFloat* releaseMs = nullptr;
    AudioParameterFloat* makeupDB  = nullptr;

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> makeupGain = 1.0f;
    const int numSteps = 200;

    LevelDetector detector;
    GainComputer gainComputer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorProcessor)
};

}
