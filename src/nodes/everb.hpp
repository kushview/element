// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#ifndef EL_REVERB_NODE_NAME
#define EL_REVERB_NODE_NAME "eVerb"
#endif

#include "nodes/baseprocessor.hpp"

namespace element {

class ReverbProcessor : public BaseProcessor
{
    AudioParameterFloat* roomSize;
    AudioParameterFloat* damping;
    AudioParameterFloat* wetLevel;
    AudioParameterFloat* dryLevel;
    AudioParameterFloat* width;

public:
    explicit ReverbProcessor()
        : BaseProcessor (BusesProperties()
                             .withInput ("Input", AudioChannelSet::stereo())
                             .withOutput ("Output", AudioChannelSet::stereo()))
    {
        addLegacyParameter (roomSize = new AudioParameterFloat ("roomSize", "Room Size", 0.0f, 1.0f, params.roomSize));
        addLegacyParameter (damping = new AudioParameterFloat ("damping", "Damping", 0.0f, 1.0f, params.damping));
        addLegacyParameter (wetLevel = new AudioParameterFloat ("wetLevel", "Wet Level", 0.0f, 1.0f, params.wetLevel));
        addLegacyParameter (dryLevel = new AudioParameterFloat ("dryLevel", "Dry Level", 0.0f, 1.0f, params.dryLevel));
        addLegacyParameter (width = new AudioParameterFloat ("width", "Width", 0.0f, 1.0f, params.width));
    }

    virtual ~ReverbProcessor() {}

    const String getName() const override { return EL_REVERB_NODE_NAME; }

    void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = "element.reverb";
        desc.descriptiveName = "Simple Reverb";
        desc.numInputChannels = 2;
        desc.numOutputChannels = 2;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    void prepareToPlay (double sampleRate, int maxBlockSize) override
    {
        setPlayConfigDetails (2, 2, sampleRate, maxBlockSize);
        verb.reset();
        verb.setSampleRate (sampleRate);
    }

    void releaseResources() override {}
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        if (paramsChanged())
        {
            params.roomSize = (float) *roomSize;
            params.damping = (float) *damping;
            params.wetLevel = (float) *wetLevel;
            params.dryLevel = (float) *dryLevel;
            params.width = (float) *width;
            verb.setParameters (params);
        }

        verb.processStereo (buffer.getWritePointer (0),
                            buffer.getWritePointer (1),
                            buffer.getNumSamples());

        lastParams.roomSize = params.roomSize;
        lastParams.damping = params.damping;
        lastParams.wetLevel = params.wetLevel;
        lastParams.dryLevel = params.dryLevel;
        lastParams.width = params.width;
    }

    void processBlockBypassed (AudioBuffer<float>& buffer, MidiBuffer& midi) override
    {
        verb.reset();
        BaseProcessor::processBlockBypassed (buffer, midi);
    }

    AudioProcessorEditor* createEditor() override { return new GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; };
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    int getNumPrograms() override { return static_cast<int> (presets().size()); };
    int getCurrentProgram() override { return currentProgram; };
    void setCurrentProgram (int index) override
    {
        if (index >= 0 && index < static_cast<int> (presets().size()))
        {
            currentProgram = index;
            const auto& preset = presets()[index];
            *roomSize = preset.params.roomSize;
            *damping = preset.params.damping;
            *wetLevel = preset.params.wetLevel;
            *dryLevel = preset.params.dryLevel;
            *width = preset.params.width;
        }
    }
    const String getProgramName (int index) override
    {
        if (index >= 0 && index < static_cast<int> (presets().size()))
        {
            return presets()[index].name;
        }
        return "Unknown";
    }
    void changeProgramName (int index, const String& newName) override { ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        ValueTree state (tags::state);
        state.setProperty ("roomSize", (float) *roomSize, nullptr);
        state.setProperty ("damping", (float) *damping, nullptr);
        state.setProperty ("wetLevel", (float) *wetLevel, nullptr);
        state.setProperty ("dryLevel", (float) *dryLevel, nullptr);
        state.setProperty ("width", (float) *width, nullptr);
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
                *roomSize = params.roomSize = ((float) state.getProperty ("roomSize", 0.0));
                *damping = params.damping = ((float) state.getProperty ("damping", 0.0));
                *wetLevel = params.wetLevel = ((float) state.getProperty ("wetLevel", 0.0));
                *dryLevel = params.dryLevel = ((float) state.getProperty ("dryLevel", 0.0));
                *width = params.width = ((float) state.getProperty ("width", 0.0));
                verb.setParameters (params);
            }
        }
    }

protected:
    bool isBusesLayoutSupported (const BusesLayout& layout) const override
    {
        return layout.getMainInputChannelSet() == AudioChannelSet::stereo()
               && layout.getMainOutputChannelSet() == AudioChannelSet::stereo();
    }

private:
    Reverb verb;
    Reverb::Parameters params, lastParams;
    int currentProgram = 0;

    struct Preset
    {
        juce::String name;
        Reverb::Parameters params;
    };

    const std::vector<Preset>& presets()
    {
        static std::vector<Preset> _presets;
        if (_presets.empty())
        {
            // Default
            _presets.push_back ({ "Default", {} });

            // Small Rooms
            _presets.push_back ({ "Small Room", { 0.3f, 0.5f, 0.33f, 0.4f, 0.5f } });
            _presets.push_back ({ "Bright Room", { 0.35f, 0.2f, 0.4f, 0.3f, 0.6f } });
            _presets.push_back ({ "Dark Room", { 0.4f, 0.8f, 0.35f, 0.35f, 0.5f } });

            // Medium Spaces
            _presets.push_back ({ "Studio", { 0.5f, 0.5f, 0.4f, 0.3f, 0.7f } });
            _presets.push_back ({ "Live Stage", { 0.55f, 0.4f, 0.45f, 0.25f, 0.8f } });
            _presets.push_back ({ "Club", { 0.6f, 0.6f, 0.5f, 0.2f, 0.75f } });

            // Large Halls
            _presets.push_back ({ "Small Hall", { 0.7f, 0.5f, 0.5f, 0.2f, 0.85f } });
            _presets.push_back ({ "Concert Hall", { 0.8f, 0.4f, 0.6f, 0.15f, 0.9f } });
            _presets.push_back ({ "Large Hall", { 0.85f, 0.5f, 0.65f, 0.1f, 0.95f } });
            _presets.push_back ({ "Cathedral", { 0.95f, 0.3f, 0.7f, 0.05f, 1.0f } });

            // Special Effects
            _presets.push_back ({ "Ambient", { 0.75f, 0.7f, 0.8f, 0.1f, 1.0f } });
            _presets.push_back ({ "Ethereal", { 0.9f, 0.2f, 0.85f, 0.05f, 1.0f } });
            _presets.push_back ({ "Plate", { 0.6f, 0.3f, 0.5f, 0.2f, 0.6f } });
            _presets.push_back ({ "Spring", { 0.4f, 0.6f, 0.4f, 0.3f, 0.4f } });

            // Creative
            _presets.push_back ({ "Tight", { 0.25f, 0.7f, 0.3f, 0.5f, 0.3f } });
            _presets.push_back ({ "Spacious", { 0.8f, 0.5f, 0.7f, 0.1f, 1.0f } });
            _presets.push_back ({ "Warm", { 0.65f, 0.75f, 0.5f, 0.25f, 0.7f } });
            _presets.push_back ({ "Shimmer", { 0.7f, 0.15f, 0.7f, 0.15f, 0.95f } });
            _presets.push_back ({ "Gated", { 0.35f, 0.9f, 0.4f, 0.4f, 0.5f } });
            _presets.push_back ({ "Reverse", { 0.5f, 0.1f, 0.6f, 0.2f, 0.8f } });

            // Instrument Specific
            _presets.push_back ({ "Vocal Booth", { 0.3f, 0.6f, 0.25f, 0.5f, 0.4f } });
            _presets.push_back ({ "Drum Room", { 0.45f, 0.5f, 0.35f, 0.4f, 0.65f } });
            _presets.push_back ({ "Piano Hall", { 0.75f, 0.4f, 0.55f, 0.2f, 0.85f } });
            _presets.push_back ({ "Guitar Cab", { 0.2f, 0.8f, 0.2f, 0.6f, 0.3f } });

            // Genre/Style
            _presets.push_back ({ "Jazz Club", { 0.5f, 0.55f, 0.45f, 0.3f, 0.7f } });
            _presets.push_back ({ "Rock Arena", { 0.7f, 0.6f, 0.6f, 0.15f, 0.9f } });
            _presets.push_back ({ "Electronic Space", { 0.85f, 0.25f, 0.75f, 0.1f, 1.0f } });
            _presets.push_back ({ "Orchestral", { 0.9f, 0.45f, 0.65f, 0.15f, 0.95f } });

            // Mixing Tools
            _presets.push_back ({ "Subtle Room", { 0.25f, 0.5f, 0.15f, 0.7f, 0.5f } });
            _presets.push_back ({ "Wide Stereo", { 0.6f, 0.4f, 0.5f, 0.25f, 1.0f } });
            _presets.push_back ({ "Mono Compatible", { 0.4f, 0.5f, 0.3f, 0.4f, 0.2f } });
            _presets.push_back ({ "Dense Tail", { 0.8f, 0.7f, 0.6f, 0.2f, 0.75f } });
        }
        return _presets;
    }

    bool paramsChanged() const noexcept
    {
        return params.roomSize != (float) *roomSize || params.damping != (float) *damping || params.wetLevel != (float) *wetLevel || params.dryLevel != (float) *dryLevel || params.width != (float) *width;
    }
};
} // namespace element
