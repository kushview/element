#include "engine/nodes/AudioFilePlayerNode.h"
#include "gui/LookAndFeel.h"
#include "Utils.h"

namespace Element {

class AudioFilePlayerEditor : public AudioProcessorEditor,
                              public FilenameComponentListener,
                              public ChangeListener,
                              public Timer
{
public:
    AudioFilePlayerEditor (AudioFilePlayerNode& o)
        : AudioProcessorEditor (&o),
          processor (o)
    {
        setOpaque (true);
        chooser.reset (new FilenameComponent ("Audio File", File(), 
                                              false, false, false,
                                              o.getWildcard(), String(),
                                              TRANS("Select Audio File")));
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
        bindHandlers();

        setSize (360, 100);
        startTimer (1001);
    }

    ~AudioFilePlayerEditor() noexcept
    {
        stopTimer();
        unbindHandlers();
        chooser = nullptr;
    }

    void timerCallback() override { stabilizeComponents(); }
    void changeListenerCallback (ChangeBroadcaster*) override { stabilizeComponents(); }
    void stabilizeComponents()
    {
        if (chooser->getCurrentFile() != processor.getAudioFile())
            chooser->setCurrentFile (processor.getAudioFile(), dontSendNotification);

        playButton.setToggleState (processor.getPlayer().isPlaying(), dontSendNotification);
        playButton.setButtonText (playButton.getToggleState() ? "Pause" : "Play");

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
    AudioFilePlayerNode& processor;
    std::unique_ptr<FilenameComponent> chooser;
    Slider position;
    Slider volume;
    TextButton playButton;
    bool draggingPos = false;

    void bindHandlers()
    {
        chooser->addListener (this);
        processor.getPlayer().addChangeListener (this);

        playButton.onClick = [this]() {
            int index = AudioFilePlayerNode::Playing;
            if (auto* playing = dynamic_cast<AudioParameterBool*> (processor.getParameters()[index]))
            {
                *playing = !*playing;
                stabilizeComponents();
            }
        };

        volume.onValueChange = [this]() {
            int index = AudioFilePlayerNode::Volume;
            if (auto* const param = dynamic_cast<AudioParameterFloat*> (processor.getParameters()[index]))
            {
                *param = static_cast<float> (volume.getValue());
                stabilizeComponents();
            }
        };

        position.onDragStart = [this]() { draggingPos = true; };
        position.onDragEnd = [this]() {
            const auto newPos = position.getValue() * processor.getPlayer().getLengthInSeconds();
            processor.getPlayer().setPosition (newPos);
            draggingPos = false;
            stabilizeComponents();
        };

        position.textFromValueFunction = [this](double value) -> String {
            const double posInMinutes = (value * processor.getPlayer().getLengthInSeconds()) / 60.0;
            return Util::minutesToString (posInMinutes);
        };
    }

    void unbindHandlers()
    {
        playButton.onClick = nullptr;
        position.onDragStart = nullptr;
        position.onDragEnd = nullptr;
        position.textFromValueFunction = nullptr;
        volume.onValueChange = nullptr;
        processor.getPlayer().removeChangeListener (this);
        chooser->removeListener (this);
    }
};

AudioFilePlayerNode::AudioFilePlayerNode()
    : BaseProcessor (BusesProperties()
        .withOutput  ("Main",  AudioChannelSet::stereo(), true))
{
    addParameter (playing = new AudioParameterBool ("playing", "Playing", false));
    addParameter (slave = new AudioParameterBool ("slave", "Slave", false));
    addParameter (volume = new AudioParameterFloat ("volume", "Volume", -60.f, 12.f, 0.f));
    for (auto* const param : getParameters())
        param->addListener (this);
}

AudioFilePlayerNode::~AudioFilePlayerNode()
{ 
    for (auto* const param : getParameters())
        param->removeListener (this);
    clearPlayer();
    playing = nullptr;
    slave = nullptr;
    volume = nullptr;
}

void AudioFilePlayerNode::fillInPluginDescription (PluginDescription& desc) const
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

void AudioFilePlayerNode::clearPlayer()
{
    player.setSource (nullptr);
    if (reader)
        reader = nullptr;
    *playing = player.isPlaying();
}

void AudioFilePlayerNode::openFile (const File& file)
{
    if (file == audioFile)
        return;
    if (auto* newReader = formats.createReaderFor (file))
    {
        clearPlayer();
        reader.reset (new AudioFormatReaderSource (newReader, true));
        audioFile = file;
        player.setSource (reader.get(), 1024 * 8, &thread, getSampleRate(), 2);
        ScopedLock sl (getCallbackLock());        
        player.setLooping (true);
        reader->setLooping (true);
    }
}

void AudioFilePlayerNode::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    thread.startThread();
    formats.registerBasicFormats();
    player.prepareToPlay (maximumExpectedSamplesPerBlock, sampleRate);
    player.setLooping (true);
    if (reader)
        reader->setLooping (true);
}

void AudioFilePlayerNode::releaseResources()
{
    player.stop();
    player.releaseResources();
    formats.clearFormats();
    thread.stopThread (14);
}

void AudioFilePlayerNode::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    const auto nframes = buffer.getNumSamples();
    for (int c = buffer.getNumChannels(); --c >= 0;)
        buffer.clear (c, 0, nframes);
    
    if (*slave)
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

AudioProcessorEditor* AudioFilePlayerNode::createEditor()
{
    return new AudioFilePlayerEditor (*this);
}

void AudioFilePlayerNode::getStateInformation (juce::MemoryBlock& destData)
{
    ValueTree state (Tags::state);
    state.setProperty ("audioFile", audioFile.getFullPathName(), nullptr)
         .setProperty ("playing", (bool)*playing, nullptr)
         .setProperty ("slave", (bool)*slave, nullptr);
    MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void AudioFilePlayerNode::setStateInformation (const void* data, int sizeInBytes)
{
    const auto state = ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (state.isValid())
    {
        if (File::isAbsolutePath (state["audioFile"].toString()))
            openFile (File (state["audioFile"].toString()));
        *playing = (bool) state.getProperty ("playing", false);
        *slave = (bool) state.getProperty ("slave", false);
    }
}

void AudioFilePlayerNode::parameterValueChanged (int parameter, float newValue)
{
    ignoreUnused (newValue);

    switch (parameter)
    {
        case Playing: {
            if (*playing)
                player.start();
            else
                player.stop();
        } break;
        
        case Slave: {
            // noop
        } break;

        case Volume: {
            player.setGain (Decibels::decibelsToGain (volume->get(), volume->range.start));
        } break;
    }
}

void AudioFilePlayerNode::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    ignoreUnused (parameterIndex, gestureIsStarting);
}

bool AudioFilePlayerNode::isBusesLayoutSupported (const BusesLayout& layout) const
{
    // only one main output bus supported. stereo or mono
    if (layout.inputBuses.size() > 0 || layout.outputBuses.size() > 1)
        return false;
    return layout.getMainOutputChannelSet() == AudioChannelSet::stereo() ||
           layout.getMainOutputChannelSet() == AudioChannelSet::mono();
}

}
