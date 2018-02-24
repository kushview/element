
#include "engine/AudioMixerProcessor.h"

namespace Element {

AudioMixerProcessor::~AudioMixerProcessor()
{
    Array<Track*> oldTracks;
    {
        ScopedLock sl (getCallbackLock());
        tracks.swapWith (oldTracks);
    }
    for (auto* t : oldTracks)
        delete t;
}

void AudioMixerProcessor::addMonoTrack()
{
    auto* const track = new Track();
    track->index = tracks.size();
    track->busIdx = -1;
    track->numInputs = 1;
    track->numOutputs = 2;
    track->lastGain = 1.0;
    track->gain = 1.0;
    track->mute = false;
    ScopedLock sl (getCallbackLock());
    tracks.add (track);
}

void AudioMixerProcessor::addStereoTrack()
{
    if (! addBus (true))
        return;
    bool wasAdded = false;
    auto* const bus = getBus (true, getBusCount (true) - 1);
    wasAdded = (bus != nullptr) ? bus->setCurrentLayout (AudioChannelSet::stereo()) : false;

    if (wasAdded)
    {
        auto* const track = new Track();
        track->index = tracks.size();
        track->busIdx = bus->getBusIndex();
        track->numInputs = 2;
        track->numOutputs = 2;
        track->lastGain = 1.0;
        track->gain = 1.0;
        track->mute = false;
        tracks.add (track);
    }
}

AudioProcessorEditor* AudioMixerProcessor::createEditor()
{
    return nullptr;
}

}
