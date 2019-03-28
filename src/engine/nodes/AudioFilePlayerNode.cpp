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

        addAndMakeVisible (startStopContinueToggle);
        startStopContinueToggle.setButtonText ("Respond to MIDI start/stop/continue");

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

        setSize (360, 122);
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

        startStopContinueToggle.setToggleState (processor.respondsToStartStopContinue(),
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
        r.removeFromTop (4);
        startStopContinueToggle.setBounds (r.removeFromTop (18));
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
    ToggleButton startStopContinueToggle;
    Atomic<int> startStopContinue { 0 };

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

        startStopContinueToggle.onClick = [this]()
        {
            processor.setRespondToStartStopContinue (
                startStopContinueToggle.getToggleState() ? 1 : 0);
            startStopContinueToggle.setToggleState (
                processor.respondsToStartStopContinue(), dontSendNotification);
            DBG("enabled: " << (int) processor.respondsToStartStopContinue());
        };
    }

    void unbindHandlers()
    {
        playButton.onClick = nullptr;
        position.onDragStart = nullptr;
        position.onDragEnd = nullptr;
        position.textFromValueFunction = nullptr;
        volume.onValueChange = nullptr;
        startStopContinueToggle.onClick = nullptr;
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

void AudioFilePlayerNode::setRespondToStartStopContinue (bool respond)
{
    midiStartStopContinue.set (respond ? 1 : 0);
}

bool AudioFilePlayerNode::respondsToStartStopContinue() const
{ 
    return midiStartStopContinue.get() == 1;
}

void AudioFilePlayerNode::fillInPluginDescription (PluginDescription& desc) const
{
    desc.name = getName();
    desc.fileOrIdentifier   = EL_INTERNAL_ID_AUDIO_FILE_PLAYER;
    desc.descriptiveName    = "A single audio file player";
    desc.numInputChannels   = 0;
    desc.numOutputChannels  = 2;
    desc.hasSharedContainer = false;
    desc.isInstrument       = false;
    desc.manufacturerName   = "Element";
    desc.pluginFormatName   = "Element";
    desc.version            = "1.0.0";
    desc.uid                = EL_INTERNAL_UID_AUDIO_FILE_PLAYER;
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
        player.setSource (reader.get(), 1024 * 8, &thread, 
                          newReader->sampleRate, 2);
        ScopedLock sl (getCallbackLock());
        reader->setLooping (true);
    }
}

void AudioFilePlayerNode::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    thread.startThread();
    formats.registerBasicFormats();
    player.prepareToPlay (maximumExpectedSamplesPerBlock, sampleRate);

    if (reader)
    {
        double readerSampleRate = sampleRate;
        if (auto* fmtReader = reader->getAudioFormatReader())
            readerSampleRate = fmtReader->sampleRate;

        reader->setLooping (true);
        player.setSource (reader.get(), 1024 * 8, &thread, readerSampleRate, 2);
        player.setPosition (jmax (0.0, lastTransportPos));
        if (wasPlaying)
            player.start();
    }
    else
    {
        clearPlayer();
    }
}

void AudioFilePlayerNode::releaseResources()
{
    lastTransportPos = player.getCurrentPosition();
    wasPlaying = player.isPlaying();

    player.stop();
    player.releaseResources();
    player.setSource (nullptr);
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

    MidiBuffer::Iterator iter (midi);
    MidiMessage msg; int frame = 0, start = 0;
    AudioSourceChannelInfo info;
    info.buffer = &buffer;

    ScopedLock sl (getCallbackLock());
    if (midiStartStopContinue.get() == 1)
    {
        while (iter.getNextEvent (msg, frame))
        {
            info.startSample = start;
            info.numSamples = frame - start;
            player.getNextAudioBlock (info);

            if (msg.isMidiStart())
            {
                midiPlayState.set (Start);
                triggerAsyncUpdate();
            } 
            else if (msg.isMidiContinue())
            {
                midiPlayState.set (Continue);
                triggerAsyncUpdate();
            }
            else if (msg.isMidiStop())
            {
                midiPlayState.set (Stop);
                triggerAsyncUpdate();
            }

            start = frame;
        }
    }

    if (start < nframes)
    {
        info.startSample = start;
        info.numSamples = nframes - start;
        player.getNextAudioBlock (info);
    }

    midi.clear();
}

void AudioFilePlayerNode::handleAsyncUpdate()
{
    switch (midiPlayState.get())
    {
        case Start:
        {
            player.setPosition (0.0);
            player.start();
        } break;

        case Stop:
        {
            player.stop();
        } break;

        case Continue:
        {
            player.start();
        } break;

        case None:
        default:
            break;
    }

    midiPlayState.set (None);
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
         .setProperty ("slave", (bool)*slave, nullptr)
         .setProperty ("midiStartStopContinue", midiStartStopContinue.get() == 1, nullptr);
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
        midiStartStopContinue.set ((bool) state.getProperty ("midiStartStopContinue", false) ? 1 : 0);
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
