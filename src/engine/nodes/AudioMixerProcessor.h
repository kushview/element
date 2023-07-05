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

namespace element {

class AudioMixerProcessor : public BaseProcessor
{
    AudioParameterBool* masterMute;
    AudioParameterFloat* masterVolume;

public:
    class Monitor : public ReferenceCountedObject
    {
    public:
        explicit Monitor (const int track, const int totalChannels = 2)
            : trackId (track), numChannels (totalChannels)
        {
            reset();
        }

        ~Monitor()
        {
            rms.clear();
        }

        inline float getGain() const { return gain.get(); }
        inline int getNumChannels() const { return numChannels; }
        inline int getTrackId() const { return trackId; }
        inline bool isMuted() const { return muted.get() > 0; }

        inline float getLevel (const int channel)
        {
            if (isPositiveAndBelow (channel, rms.size()))
                return rms.getReference (channel).get();
            return 0.f;
        }

        inline void requestMute (const bool muted)
        {
            nextMute.set (muted ? 1 : 0);
        }

        inline void requestGain (const float gain)
        {
            nextGain.set (gain);
        }

        inline void requestVolume (const float dB)
        {
            requestGain (Decibels::decibelsToGain (dB, -120.f));
        }

    private:
        friend class AudioMixerProcessor;
        const int trackId;
        const int numChannels;
        Array<Atomic<float>> rms;
        Atomic<int> muted;
        Atomic<int> nextMute;
        Atomic<float> gain;
        Atomic<float> nextGain;

        void reset()
        {
            muted = 0;
            nextMute = 0;
            gain = 1.f;
            nextGain = 1.f;
            if (rms.size() > 0)
                rms.clearQuick();
            while (rms.size() < numChannels)
                rms.add (Atomic<float> (0.f));
        }
    };

    typedef ReferenceCountedObjectPtr<Monitor> MonitorPtr;

    struct Track
    {
        int index = -1;
        int busIdx = -1;
        int numInputs = 0;
        int numOutputs = 0;
        float lastGain = 1.0;
        float gain = 1.0;
        bool mute = false;
        MonitorPtr monitor;

        inline void update (const Track* const track)
        {
            this->index = track->index;
            this->busIdx = track->busIdx;
            this->numInputs = track->numInputs;
            this->numOutputs = track->numOutputs;
            this->gain = track->gain;
            this->lastGain = track->gain;
            this->mute = track->mute;
            this->monitor = track->monitor;
        }
    };

    explicit AudioMixerProcessor (int numTracks = 4,
                                  const double sampleRate = 44100.0,
                                  const int bufferSize = 1024)
        : BaseProcessor (BusesProperties()
                             .withOutput ("Master", AudioChannelSet::stereo(), false))
    {
        tracks.ensureStorageAllocated (16);
        while (--numTracks >= 0)
            addStereoTrack();

        setRateAndBufferSizeDetails (sampleRate, bufferSize);
        addLegacyParameter (masterMute = new AudioParameterBool ("masterMute", "Master Mute", false));
        addLegacyParameter (masterVolume = new AudioParameterFloat ("masterVolume", "Master Volume", -120.0f, 12.0f, 0.f));
        masterMonitor = new Monitor (-1, 2);
    }

    ~AudioMixerProcessor();

    inline const String getName() const override { return "Audio Mixer"; }

    inline void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier = "element.audioMixer";
        desc.descriptiveName = "Simple 4 track mixer";
        desc.category = "Mixer";
        desc.numInputChannels = getTotalNumInputChannels();
        desc.numOutputChannels = getTotalNumOutputChannels();
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    int getNumTracks() const
    {
        ScopedLock sl (getCallbackLock());
        return tracks.size();
    }

    MonitorPtr getMonitor (const int track = -1) const;

    void setTrackGain (const int track, const float gain);
    void setTrackMuted (const int track, const bool mute);
    bool isTrackMuted (const int track) const;
    float getTrackGain (const int track) const;

    inline bool acceptsMidi() const override { return false; }
    inline bool producesMidi() const override { return false; }

    inline bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    void prepareToPlay (const double, const int) override;
    void processBlock (AudioSampleBuffer& audio, MidiBuffer& midi) override;
    void releaseResources() override;

    inline bool isBusesLayoutSupported (const BusesLayout& layout) const override
    {
        for (const auto& bus : layout.inputBuses)
            if (bus != layout.getMainOutputChannelSet())
                return false;
        for (const auto& bus : layout.outputBuses)
            if (bus != layout.getMainOutputChannelSet())
                return false;
        return true;
    }

    bool canAddBus (bool) const override { return true; }
    bool canRemoveBus (bool) const override { return true; }
    bool canApplyBusCountChange (bool isInput, bool isAdding, AudioProcessor::BusProperties& outProperties) override;

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int) override {}
    const String getProgramName (int) override { return "Audio Mixer"; }
    void changeProgramName (int, const String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

private:
    MonitorPtr masterMonitor;
    Array<Track*> tracks;
    int numTracks = 0;
    AudioSampleBuffer tempBuffer;
    float lastGain = 0.f;
    void addMonoTrack();
    void addStereoTrack();
};

} // namespace element
