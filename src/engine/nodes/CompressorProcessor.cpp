// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// Author: Jatin Chowdhury (jatin@ccrma.stanford.edu)
// SPDX-License-Identifier: GPL3-or-later

#include "CompressorProcessor.h"
#include "gui/nodes/CompressorNodeEditor.h"

namespace element {

CompressorProcessor::CompressorProcessor (const int _numChannels)
    : BaseProcessor (BusesProperties()
                         .withInput ("Main", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))
                         .withInput ("Sidechain", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))
                         .withOutput ("Main", AudioChannelSet::canonicalChannelSet (jlimit (1, 2, _numChannels)))),
      numChannels (jlimit (1, 2, _numChannels))
{
    setBusesLayout (getBusesLayout());
    setRateAndBufferSizeDetails (44100.0, 1024);

    NormalisableRange<float> ratioRange (0.5f, 10.0f);
    ratioRange.setSkewForCentre (2.0f);

    NormalisableRange<float> attackRange (0.1f, 1000.0f);
    attackRange.setSkewForCentre (10.0f);

    NormalisableRange<float> releaseRange (10.0f, 3000.0f);
    releaseRange.setSkewForCentre (100.0f);

    addLegacyParameter (threshDB = new AudioParameterFloat ("thresh", "Threshold [dB]", -30.0f, 0.0f, 0.0f));
    addLegacyParameter (ratio = new AudioParameterFloat ("ratio", "Ratio", ratioRange, 1.0f));
    addLegacyParameter (kneeDB = new AudioParameterFloat ("knee", "Knee [dB]", 0.0f, 12.0f, 6.0f));
    addLegacyParameter (attackMs = new AudioParameterFloat ("attack", "Attack [ms]", attackRange, 10.0f));
    addLegacyParameter (releaseMs = new AudioParameterFloat ("release", "Release [ms]", releaseRange, 100.0f));
    addLegacyParameter (makeupDB = new AudioParameterFloat ("makeup", "Makeup [dB]", -18.0f, 18.0f, 0.0f));
    addLegacyParameter (sideChain = new AudioParameterFloat ("sidechain", "Side Chain", 0.0f, 1.0f, 0.0f));

    makeupGain.reset (numSteps);
}

void CompressorProcessor::fillInPluginDescription (PluginDescription& desc) const
{
    desc.name = getName();
    desc.fileOrIdentifier = EL_NODE_ID_COMPRESSOR;
    desc.descriptiveName = "Compressor";
    desc.numInputChannels = numChannels * 2;
    desc.numOutputChannels = numChannels;
    desc.hasSharedContainer = false;
    desc.isInstrument = false;
    desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
    desc.pluginFormatName = "Element";
    desc.version = "1.0.0";
    desc.uniqueId = EL_NODE_UID_COMPRESSOR;
}

void CompressorProcessor::updateParams()
{
    detector.setAttackMs (*attackMs);
    detector.setReleaseMs (*releaseMs);

    sideDetector.setAttackMs (*attackMs);
    sideDetector.setReleaseMs (*releaseMs);

    gainComputer.setThreshold (*threshDB);
    gainComputer.setRatio (*ratio);
    gainComputer.setKnee (*kneeDB);

    makeupGain.setTargetValue (Decibels::decibelsToGain ((float) *makeupDB));
}

void CompressorProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    detector.reset ((float) sampleRate);
    sideDetector.reset ((float) sampleRate);
    gainComputer.reset();

    setBusesLayout (getBusesLayout());
    setRateAndBufferSizeDetails (sampleRate, maximumExpectedSamplesPerBlock);
}

void CompressorProcessor::releaseResources() {}

void CompressorProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    auto mainBuffer = getBusBuffer (buffer, true, 0);
    auto sideBuffer = getBusBuffer (buffer, true, 1);

    auto numBuffChannels = mainBuffer.getNumChannels();

    updateParams();

    float level = 0.0f;
    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        // Get Main mono input
        float mainInput = getSummedMonoSample (mainBuffer, n, numBuffChannels);

        // Get sidechain mono input
        float sideInput = getSummedMonoSample (sideBuffer, n, numBuffChannels);

        // Get level estimate, and compute gain
        level = detector.process (mainInput) * (1.0f - *sideChain)
                + sideDetector.process (sideInput) * *sideChain;
        float gain = gainComputer.process (level);

        // Apply gain
        mainBuffer.applyGain (n, 1, gain * makeupGain.getNextValue());
    }

    listeners.call (&Listener::updateInGainDB, Decibels::gainToDecibels (level));
}

float CompressorProcessor::calcGainDB (float db)
{
    auto x = Decibels::decibelsToGain (db);
    auto gain = gainComputer.calcGain (x, gainComputer.thresh.getCurrentValue(), gainComputer.ratio.getCurrentValue());
    return Decibels::gainToDecibels (gain);
}

AudioProcessorEditor* CompressorProcessor::createEditor() { return new CompressorNodeEditor (*this); }

void CompressorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state (tags::state);
    state.setProperty ("thresh", (float) *threshDB, 0);
    state.setProperty ("ratio", (float) *ratio, 0);
    state.setProperty ("knee", (float) *kneeDB, 0);
    state.setProperty ("attack", (float) *attackMs, 0);
    state.setProperty ("release", (float) *releaseMs, 0);
    state.setProperty ("makeup", (float) *makeupDB, 0);
    state.setProperty ("sidechain", (float) *sideChain, 0);
    if (auto e = state.createXml())
        AudioProcessor::copyXmlToBinary (*e, destData);
}

void CompressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto e = AudioProcessor::getXmlFromBinary (data, sizeInBytes))
    {
        auto state = ValueTree::fromXml (*e);
        if (state.isValid())
        {
            *threshDB = (float) state.getProperty ("thresh", (float) *threshDB);
            *ratio = (float) state.getProperty ("ratio", (float) *ratio);
            *kneeDB = (float) state.getProperty ("knee", (float) *kneeDB);
            *attackMs = (float) state.getProperty ("attack", (float) *attackMs);
            *releaseMs = (float) state.getProperty ("release", (float) *releaseMs);
            *makeupDB = (float) state.getProperty ("makeup", (float) *makeupDB);
            *sideChain = (float) state.getProperty ("sidechain", (float) *sideChain);
        }
    }
}

void CompressorProcessor::numChannelsChanged()
{
    numChannels = getTotalNumInputChannels();
}

} // namespace element
