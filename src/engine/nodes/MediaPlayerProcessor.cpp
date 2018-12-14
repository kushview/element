#include "engine/nodes/MediaPlayerProcessor.h"

namespace Element {

MediaPlayerProcessor::MediaPlayerProcessor()
    : BaseProcessor (BusesProperties()
        .withOutput  ("Main",  AudioChannelSet::stereo(), true))
{
    addParameter (playing = new AudioParameterBool ("playing", "Playing", false));
    for (auto* const param : getParameters())
        param->addListener (this);
}

MediaPlayerProcessor::~MediaPlayerProcessor()
{ 
    for (auto* const param : getParameters())
        param->removeListener (this);
    clearPlayer();
    playing = nullptr;
}

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

void MediaPlayerProcessor::clearPlayer()
{
    player.setSource (nullptr);
    if (reader)
        reader = nullptr;
}

void MediaPlayerProcessor::openFile (const File& file)
{
    if (auto* newReader = formats.createReaderFor (file))
    {
        clearPlayer();
        reader.reset (new AudioFormatReaderSource (newReader, true));
        player.setSource (reader.get(), 1024 * 8, &thread, getSampleRate(), 2);
    }
}

void MediaPlayerProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    thread.startThread();
    formats.registerBasicFormats();
    player.prepareToPlay (maximumExpectedSamplesPerBlock, sampleRate);
    openFile (File ("/Users/mfisher/Desktop/Music/Jackson 5-Ill Be There.wav"));
    player.setLooping (true);
    player.start();
}

void MediaPlayerProcessor::releaseResources()
{
    player.stop();
    player.releaseResources();
    formats.clearFormats();
    thread.stopThread (50);
}

void MediaPlayerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    buffer.clear (0, 0, buffer.getNumSamples());
    buffer.clear (1, 0, buffer.getNumSamples());
    const AudioSourceChannelInfo info (buffer);
    player.getNextAudioBlock (info);
    midi.clear();
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

void MediaPlayerProcessor::parameterValueChanged (int parameterIndex, float newValue)
{
    if (parameterIndex == 0)
    {
        if (*playing)
            player.start();
        else
            player.stop();
    }
}

void MediaPlayerProcessor::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    ignoreUnused (parameterIndex, gestureIsStarting);
}

}
