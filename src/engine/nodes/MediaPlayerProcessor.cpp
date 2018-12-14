#include "engine/nodes/MediaPlayerProcessor.h"

namespace Element {

MediaPlayerProcessor::MediaPlayerProcessor()
    : BaseProcessor (BusesProperties()
        .withOutput  ("Main",  AudioChannelSet::stereo(), true))
{ }

MediaPlayerProcessor::~MediaPlayerProcessor() { }

void MediaPlayerProcessor::fillInPluginDescription (PluginDescription& desc) const
{
    desc.name = getName();
    desc.fileOrIdentifier   = EL_INTERNAL_ID_MEDIA_PLAYER;
    desc.descriptiveName    = EL_INTERNAL_ID_MEDIA_PLAYER;
    desc.numInputChannels   = 0;
    desc.numOutputChannels  = 2;
    desc.hasSharedContainer = false;
    desc.isInstrument       = false;
    desc.manufacturerName   = "Element";
    desc.pluginFormatName   = "Element";
    desc.version            = "1.0.0";
}

void MediaPlayerProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    
}

void MediaPlayerProcessor::releaseResources()
{
    
}

void MediaPlayerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    buffer.clear();
    midiMessages.clear();
}

AudioProcessorEditor* MediaPlayerProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (this);
}

void MediaPlayerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    ignoreUnused (destData);
}

void MediaPlayerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    ignoreUnused (data, sizeInBytes);
}

}
