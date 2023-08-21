// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "nodes/baseprocessor.hpp"
#include "ElementApp.h"

namespace element {

/**
    A simple level detector with exponential attack characteristic,
    and exponential, return-to-zero release characteristic.
*/
class LevelDetector
{
public:
    LevelDetector() {}

    void setAttackMs (float newAttackMs)
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
        ratio.reset (numSteps);
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

    inline float calcGain (float x, float curThresh, float curRatio)
    {
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

    inline float process (float x)
    {
        return calcGain (x, thresh.getNextValue(), ratio.getNextValue());
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

    friend class CompressorProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainComputer)
};

/** Compressor Processing */
class CompressorProcessor : public BaseProcessor
{
public:
    explicit CompressorProcessor (const int _numChannels = 2);

    const String getName() const override { return "Compressor"; }

    void fillInPluginDescription (PluginDescription& desc) const override;

    void updateParams();
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override;
    float calcGainDB (float db);

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

    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void updateInGainDB (float /*inDB*/) {}
    };

    void addCompressorListener (Listener* l) { listeners.add (l); }
    void removeCompressorListener (Listener* l) { listeners.remove (l); }

protected:
    inline bool isBusesLayoutSupported (const BusesLayout& layout) const override
    {
        // supports two input buses, one output
        if (layout.inputBuses.size() != 2 && layout.outputBuses.size() != 1)
            return false;

        // ins must equal outs
        if (layout.getMainInputChannels() != layout.getMainOutputChannels())
            return false;

        // check sidechain input channels is equal to main
        if (layout.getMainInputChannels() != layout.inputBuses[1].size())
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
    inline float getSummedMonoSample (AudioBuffer<float>& buffer, int idx, int numChans)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numChans; ++ch)
            sum += buffer.getSample (ch, idx);
        return sum / (float) numChans;
    };

    int numChannels = 0;
    AudioParameterFloat* threshDB = nullptr;
    AudioParameterFloat* ratio = nullptr;
    AudioParameterFloat* kneeDB = nullptr;
    AudioParameterFloat* attackMs = nullptr;
    AudioParameterFloat* releaseMs = nullptr;
    AudioParameterFloat* makeupDB = nullptr;
    AudioParameterFloat* sideChain = nullptr;

    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> makeupGain = 1.0f;
    const int numSteps = 200;

    LevelDetector detector;
    LevelDetector sideDetector;
    GainComputer gainComputer;

    ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorProcessor)
};

} // namespace element
