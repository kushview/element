
#pragma once

#include "engine/BaseProcessor.h"

namespace Element {

class AudioMixerProcessor : public BaseProcessor
{
public:
    explicit AudioMixerProcessor (int numTracks = 4)
        : BaseProcessor (BusesProperties()
            .withOutput ("Master",  AudioChannelSet::stereo(), false))
    {
        while (--numTracks >= 0)
            addStereoTrack();
        setRateAndBufferSizeDetails (44100.0, 1024);
    }

    ~AudioMixerProcessor();

    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    void addMonoTrack();
    void addStereoTrack();

private:
    struct Track
    {
        int index = -1;
        int busIdx = -1;
        int numInputs = 0;
        int numOutputs = 0;
        float lastGain = 1.0;
        float gain = 1.0;
        bool mute = false;
    };
    Array<Track*> tracks;
};

}
