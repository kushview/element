
#include "engine/AudioMixerProcessor.h"

namespace Element {

AudioMixerProcessor::~AudioMixerProcessor()
{
    Array<Track*> oldTracks;
    {
        ScopedLock sl (getCallbackLock());
        masterMute = nullptr;
        masterVolume = nullptr;
        tracks.swapWith (oldTracks);
    }

    for (auto* t : oldTracks)
        delete t;
}

void AudioMixerProcessor::addMonoTrack()
{
    auto* track = new Track();
    track->index = tracks.size();
    track->busIdx = -1;
    track->numInputs = 1;
    track->numOutputs = 2;
    track->lastGain = 1.0;
    track->gain = 1.0;
    track->mute = false;
    deleteAndZero (track); // mono not yet supported
}

void AudioMixerProcessor::addStereoTrack()
{
    if (! addBus (true))
        return;

    bool wasAdded = false;
    auto* const input = getBus (true, getBusCount (true) - 1);
    
    if (input != nullptr)
        wasAdded = input->setCurrentLayout (AudioChannelSet::stereo());

    if (wasAdded)
    {
        auto* const track = new Track();
        track->index        = tracks.size();
        track->busIdx       = input->getBusIndex();
        track->numInputs    = input->getNumberOfChannels();
        track->numOutputs   = input->getNumberOfChannels();
        track->lastGain     = 1.0;
        track->gain         = 1.0;
        track->mute         = false;

        ScopedLock sl (getCallbackLock());
        tracks.add (track);
    }
    else
    {
        DBG("[EL] AudioMixerProcessor: could not add new track");
    }
}

AudioProcessorEditor* AudioMixerProcessor::createEditor()
{
    return nullptr;
}

void AudioMixerProcessor::prepareToPlay (const double sampleRate, const int bufferSize)
{
    setRateAndBufferSizeDetails (sampleRate, bufferSize);
    jassert (tracks.size() == getBusCount (true));
    jassert (1 == getBusCount (false));
    tempBuffer.setSize (getMainBusNumOutputChannels(), bufferSize, false, true, true);
}

void AudioMixerProcessor::processBlock (AudioSampleBuffer& audio, MidiBuffer& midi)
{
    midi.clear();

    ScopedLock sl (getCallbackLock());

    if (tracks.size() <= 0)
    {
        audio.clear();
        return;
    }

    auto output (getBusBuffer<float> (audio, false, 0));
    const int numSamples = audio.getNumSamples();
    tempBuffer.clear (0, numSamples);

    for (int i = 0; i < tracks.size(); ++i)
    {
        auto* const track = tracks.getUnchecked (i);
        auto input (getBusBuffer<float> (audio, true, track->busIdx));
        if (! track->mute)
        {
            for (int c = 0; c < output.getNumChannels(); ++c)
                tempBuffer.addFromWithRamp (c, 0, input.getReadPointer(c), numSamples,
                                            track->lastGain, track->gain);
        }
        track->lastGain = track->gain;
    }

    output.clear (0, audio.getNumSamples());
    const float gain = Decibels::decibelsToGain ((float)*masterVolume, -120.f);
    if (! *masterMute)
        for (int c = 0; c < output.getNumChannels(); ++c)
            output.copyFromWithRamp (c, 0, tempBuffer.getReadPointer(c), numSamples,
                                     lastGain, gain);
    lastGain = gain;
}

void AudioMixerProcessor::releaseResources()
{
    tempBuffer.setSize (1, 1, false, false, false);
}

bool AudioMixerProcessor::canApplyBusCountChange (bool isInput, bool isAdding,
                                                  AudioProcessor::BusProperties& outProperties)
{
    if (  isAdding && ! canAddBus    (isInput)) return false;
    if (! isAdding && ! canRemoveBus (isInput)) return false;

    auto num = getBusCount (isInput);
    auto* const main = getBus (false, 0);
    if (! main)
        return false;
    
    if (isAdding)
    {
        outProperties.busName = String (isInput ? "Input #" : "Output #") + String (getBusCount (isInput));
        outProperties.defaultLayout = (num > 0 ? getBus (isInput, num - 1)->getDefaultLayout() 
                                               : main->getDefaultLayout());
        outProperties.isActivatedByDefault = true;
    }

    return true;
}

}
