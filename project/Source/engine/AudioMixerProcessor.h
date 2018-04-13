
#pragma once

#include "engine/BaseProcessor.h"

namespace Element {

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
            muted = 0;
            while (rms.size() < numChannels)
                rms.add (Atomic<float> (0.f));
        }

        ~Monitor()
        {
            rms.clear();
        }

        int getNumChannels() const { return numChannels; }
        int getTrackId() const { return trackId; }

        bool isMuted() const { return muted.get() > 0; }

        float getLevel (const int channel)
        {
            if (isPositiveAndBelow (channel, rms.size()))
                return rms.getReference(channel).get();
            return 0.f;
        }

    private:
        friend class AudioMixerProcessor;
        const int trackId;
        const int numChannels;
        Array<Atomic<float> > rms;
        Atomic<int> muted;
        Atomic<float> gain;
    };

    typedef ReferenceCountedObjectPtr<Monitor> MonitorPtr;

    explicit AudioMixerProcessor (int numTracks = 4,
                                  const double sampleRate = 44100.0,
                                  const int bufferSize = 1024)
        : BaseProcessor (BusesProperties()
            .withOutput ("Master",  AudioChannelSet::stereo(), false))
    {
        tracks.ensureStorageAllocated (16);
        while (--numTracks >= 0)
            addStereoTrack();
        setRateAndBufferSizeDetails (sampleRate, bufferSize);
        addParameter (masterMute = new AudioParameterBool ("masterMute", "Room Size", false));
        addParameter (masterVolume  = new AudioParameterFloat ("masterVolume",  "Damping", -120.0f, 12.0f, 0.f));
    }

    ~AudioMixerProcessor();

    inline const String getName() const override { return "Audio Mixer"; }
    inline void fillInPluginDescription (PluginDescription& desc) const override
    {
        desc.name = getName();
        desc.fileOrIdentifier   = "element.audioMixer";
        desc.descriptiveName    = "Audio Mixer";
        desc.numInputChannels   = getTotalNumInputChannels();
        desc.numOutputChannels  = getTotalNumOutputChannels();
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Kushview";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }

    int getNumTracks() const { ScopedLock sl (getCallbackLock()); return tracks.size(); }
    
    MonitorPtr getMonitor (const int track = -1) const;
    void setTrackGain  (const int track, const float gain);
    void setTrackMuted (const int track, const bool mute);
    bool isTrackMuted  (const int track) const;
    float getTrackGain  (const int track) const;

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
    bool canApplyBusCountChange (bool isInput, bool isAdding,
                                 AudioProcessor::BusProperties& outProperties) override;

    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 1; }
    void setCurrentProgram (int) override { }
    const String getProgramName (int) override { return "Program"; }
    void changeProgramName (int, const String&) override { }
    void getStateInformation (juce::MemoryBlock&) override { }
    void setStateInformation (const void*, int) override { }

private:
    struct Track
    {
        int index       = -1;
        int busIdx      = -1;
        int numInputs   = 0;
        int numOutputs  = 0;
        float lastGain  = 1.0;
        float gain      = 1.0;
        bool mute       = false;
        MonitorPtr      monitor;
    };

    MonitorPtr masterMonitor;
    Array<Track*> tracks;
    int numTracks = 0;
    AudioSampleBuffer tempBuffer;
    float lastGain = 0.f;
    void addMonoTrack();
    void addStereoTrack();
};

}
