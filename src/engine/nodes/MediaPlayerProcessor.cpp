#include "engine/nodes/MediaPlayerProcessor.h"
#include "gui/LookAndFeel.h"

namespace Element {

class MediaPlayerEditor : public AudioProcessorEditor,
                          public FilenameComponentListener,
                          public ChangeListener,
                          public Timer
{
public:
    MediaPlayerEditor (MediaPlayerProcessor& o)
        : AudioProcessorEditor (&o),
          processor (o)
    {
        setOpaque (true);
        chooser.reset (new FilenameComponent ("Audio File", File(), 
                                              false, false, false, 
                                              "*.wav;*.aif;*.aiff", 
                                              String(),
                                              "Select Audio File"));
        chooser->addListener (this);
        addAndMakeVisible (chooser.get());
        
        addAndMakeVisible (playButton);
        playButton.setButtonText ("Play");
        addAndMakeVisible (position);
        position.setSliderStyle (Slider::LinearBar);
        position.setRange (0.0, 1.0, 0.001);
        position.setTextBoxIsEditable (false);

        addAndMakeVisible (volume);
        volume.setSliderStyle (position.getSliderStyle());
        volume.setRange (-60.0, 12.0, 0.1);
        volume.setTextBoxIsEditable (false);

        stabilizeComponents();
        
        processor.getPlayer().addChangeListener (this);

        playButton.onClick = [this]() {
            if (auto* playing = dynamic_cast<AudioParameterBool*> (processor.getParameters()[0]))
            {
                *playing = ! playButton.getToggleState();
                playButton.setToggleState (*playing, dontSendNotification);
                stabilizeComponents();
            }
        };

        volume.onValueChange = [this]() {
            processor.getPlayer().setGain (
                Decibels::decibelsToGain (volume.getValue(), volume.getMinimum()));
        };

        position.onDragStart = [this]() { draggingPos = true; };
        position.onDragEnd = [this]() {
            const auto newPos = position.getValue() * processor.getPlayer().getLengthInSeconds();
            processor.getPlayer().setPosition (newPos);
            draggingPos = false;
            stabilizeComponents();
        };

        position.textFromValueFunction = [this](double value) -> String {
            const double pos = (value * processor.getPlayer().getLengthInSeconds()) / 60.0;
            double minutes = 0, seconds = 60.0 * modf (pos, &minutes);
            String ms (roundToInt (floor (minutes)));
            String mm (roundToInt (floor (seconds)));
            return ms.paddedLeft('0', 2) + ":" + mm.paddedLeft('0', 2);
        };

        setSize (360, 180);

        startTimer (500);
    }

    ~MediaPlayerEditor() noexcept
    {
        stopTimer();
        processor.getPlayer().removeChangeListener (this);
        playButton.onClick = nullptr;
        position.onDragEnd = nullptr;
        volume.onValueChange = nullptr;
        chooser->removeListener (this);
        chooser = nullptr;
    }

    void timerCallback() override { stabilizeComponents(); }
    void changeListenerCallback (ChangeBroadcaster*) override { stabilizeComponents(); }
    void stabilizeComponents()
    {
        if (auto* playing = dynamic_cast<AudioParameterBool*> (processor.getParameters()[0]))
        {
            playButton.setToggleState (*playing || processor.getPlayer().isPlaying(), dontSendNotification);
            playButton.setButtonText (playButton.getToggleState() ? "Pause" : "Play");
        }

        if (! draggingPos)
        {
            if (processor.getPlayer().getLengthInSeconds() > 0.0)
            {
                position.setValue (
                    processor.getPlayer().getCurrentPosition() / processor.getPlayer().getLengthInSeconds(),
                    dontSendNotification);
            }
            else
            {
                position.setValue (position.getMinimum(), dontSendNotification);
            }
        }
        
        volume.setValue (
            (double) Decibels::gainToDecibels ((double) processor.getPlayer().getGain(), (double) volume.getMinimum()), 
            dontSendNotification);
    }

    void filenameComponentChanged (FilenameComponent*) override
    {
        processor.openFile (chooser->getCurrentFile());
    }

    void resized() override
    {
        auto r (getLocalBounds().reduced (4));
        chooser->setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        playButton.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        volume.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        position.setBounds (r.removeFromTop (18));
    }

    void paint (Graphics& g) override
    {
        g.fillAll (LookAndFeel::widgetBackgroundColor);
    }

private:
    MediaPlayerProcessor& processor;
    std::unique_ptr<FilenameComponent> chooser;
    Slider position;
    Slider volume;
    TextButton playButton;
    bool draggingPos = false;
};

MediaPlayerProcessor::MediaPlayerProcessor()
    : BaseProcessor (BusesProperties()
        .withOutput  ("Main",  AudioChannelSet::stereo(), true))
{
    addParameter (playing = new AudioParameterBool ("playing", "Playing", false));
    addParameter (slave = new AudioParameterBool ("slave", "Slave", false));
    addParameter (new AudioParameterFloat ("volume", "Volume", -60.f, 12.f, 0.f));
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
    player.setLooping (true);
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

    if (*sync)
    {
        if (auto* const playhead = getPlayHead())
        {
            AudioPlayHead::CurrentPositionInfo pos;
            playhead->getCurrentPosition (pos);
        }
    }

    const AudioSourceChannelInfo info (buffer);
    player.getNextAudioBlock (info);
    midi.clear();
}

AudioProcessorEditor* MediaPlayerProcessor::createEditor()
{
    return new MediaPlayerEditor (*this);
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
