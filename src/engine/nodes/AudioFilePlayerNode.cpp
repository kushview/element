/*
    This file is part of Element
    Copyright (C) 2019-2021  Kushview, LLC.  All rights reserved.

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

#include "engine/nodes/AudioFilePlayerNode.h"
#include "gui/LookAndFeel.h"
#include "gui/ViewHelpers.h"

// nav panel needs these headers included
#include "controllers/EngineController.h"
#include "gui/AudioIOPanelView.h"
#include "gui/SessionTreePanel.h"
#include "gui/views/PluginsPanelView.h"
#include "gui/NavigationConcertinaPanel.h"

#include "Utils.h"

namespace Element {

class AudioFilePlayerEditor : public AudioProcessorEditor,
                              public FileComboBoxListener,
                              public ChangeListener,
                              public DragAndDropTarget,
                              public FileDragAndDropTarget,
                              public Timer
{
public:
    AudioFilePlayerEditor (AudioFilePlayerNode& o)
        : AudioProcessorEditor (&o),
          processor (o)
    {
        setOpaque (true);
        chooser.reset (new FileComboBox ("Audio File", File(), 
                                              false, false, false,
                                              o.getWildcard(), String(),
                                              TRANS("Select Audio File")));
        addAndMakeVisible (chooser.get());
        chooser->setShowFullPathName (false);

        addAndMakeVisible (watchButton); 
        watchButton.setIcon (Icon (getIcons().fasFolderOpen, Colours::black));
        
        addAndMakeVisible (playButton);
        playButton.setButtonText ("Play");

        addAndMakeVisible (loopButton);
        loopButton.setButtonText ("Loop");
        loopButton.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);

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

        setSize (360, 144);
        startTimer (1001);
    }

    ~AudioFilePlayerEditor() noexcept
    {
        stopTimer();
        unbindHandlers();
        chooser = nullptr;
    }

    void addRecentsFrom (const File& recentsDir, bool recursive = true)
    {
        if (recentsDir.isDirectory())
        {
            DirectoryIterator iter (recentsDir, recursive, processor.getWildcard(), File::findFiles);
            while (iter.next())
            {
                if (iter.getFile().isDirectory())
                    continue;
                chooser->addRecentlyUsedFile (iter.getFile());
            }

            sortRecents();
        }
    }

    void timerCallback() override { stabilizeComponents(); }
    void changeListenerCallback (ChangeBroadcaster*) override { stabilizeComponents(); }

    void stabilizeComponents()
    {
        if (processor.getWatchDir().isDirectory())
            if (chooser->getRecentlyUsedFilenames().isEmpty())
                addRecentsFrom (processor.getWatchDir());

        if (chooser->getCurrentFile() != processor.getAudioFile())
            if (processor.getAudioFile().existsAsFile())
                chooser->setCurrentFile (processor.getAudioFile(), dontSendNotification);

        playButton.setToggleState (processor.getPlayer().isPlaying(), dontSendNotification);
        playButton.setButtonText (playButton.getToggleState() ? "Pause" : "Play");

        loopButton.setToggleState (processor.isLooping(), dontSendNotification);

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

    void fileComboBoxChanged (FileComboBox*) override
    {
        const auto f1 = chooser->getCurrentFile();
        const auto f2 = processor.getAudioFile();
        DBG(f1.getFullPathName());

        if (! f1.isDirectory() && f1 != f2)
            processor.openFile (chooser->getCurrentFile());
    }

    void resized() override
    {
        auto r (getLocalBounds().reduced (4));
        auto r2 = r.removeFromTop (18);

        watchButton.setBounds (r2.removeFromRight (22));
        chooser->setBounds (r2);

        r.removeFromTop (4);
        playButton.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        loopButton.setBounds (r.removeFromTop (18));
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

    //=========================================================================
    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        if (details.description.toString() == "ccNavConcertinaPanel")
            return true;
        return false;
    }

    void itemDropped (const SourceDetails& details) override 
    {
        if (details.description.toString() == "ccNavConcertinaPanel")
        {
            auto* const nav = ViewHelpers::getNavigationConcertinaPanel (this);
            if (auto* panel = (nav) ? nav->findPanel<DataPathTreeComponent>() : nullptr)
            {
                File file = panel->getSelectedFile();
                if (processor.canLoad (file))
                    processor.openFile (file);
            }
        }
    }
   #if 0
    virtual void itemDragEnter (const SourceDetails& dragSourceDetails);
    virtual void itemDragMove (const SourceDetails& dragSourceDetails);
    virtual void itemDragExit (const SourceDetails& dragSourceDetails);
    virtual bool shouldDrawDragImageWhenOver();
   #endif

    //=========================================================================
    bool isInterestedInFileDrag (const StringArray& files) override
    {
        if (! File::isAbsolutePath (files[0]))
            return false;
        return processor.canLoad (File (files [0]));
    }

    void filesDropped (const StringArray& files, int x, int y) override
    {
        ignoreUnused (x, y);
        processor.openFile (File (files [0]));
    }
    
   #if 0
    virtual void fileDragEnter (const StringArray& files, int x, int y);
    virtual void fileDragExit (const StringArray& files);
   #endif

private:
    AudioFilePlayerNode& processor;
    std::unique_ptr<FileComboBox> chooser;
    Slider position;
    Slider volume;
    TextButton playButton;
    TextButton loopButton;
    IconButton watchButton;
    ToggleButton startStopContinueToggle;
    Atomic<int> startStopContinue { 0 };
    SignalConnection stateRestoredConnection;
    
    bool draggingPos = false;

    void sortRecents()
    {
        auto names = chooser->getRecentlyUsedFilenames();
        names.sort (false);
        chooser->setRecentlyUsedFilenames (names);
    }

    void bindHandlers()
    {
        processor.getPlayer().addChangeListener (this);
        stateRestoredConnection = processor.restoredState.connect (std::bind(
            &AudioFilePlayerEditor::onStateRestored, this
        ));

        chooser->addListener (this);
        watchButton.onClick = [this]() {
            FileChooser fc ("Select a folder to watch", File(), "*", true, false, nullptr);
            if (fc.browseForDirectory())
            {
                processor.setWatchDir (fc.getResult());
                addRecentsFrom (processor.getWatchDir());
            }
        };

        playButton.onClick = [this]() {
            int index = AudioFilePlayerNode::Playing;
            if (auto* playing = dynamic_cast<AudioParameterBool*> (processor.getParameters()[index]))
            {
                *playing = !*playing;
                stabilizeComponents();
            }
        };

        loopButton.onClick = [this]()
        {
            processor.setLooping (! processor.isLooping());
            stabilizeComponents();
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
        };
    }

    void unbindHandlers()
    {
        stateRestoredConnection.disconnect();
        playButton.onClick = nullptr;
        loopButton.onClick = nullptr;
        position.onDragStart = nullptr;
        position.onDragEnd = nullptr;
        position.textFromValueFunction = nullptr;
        volume.onValueChange = nullptr;
        startStopContinueToggle.onClick = nullptr;
        processor.getPlayer().removeChangeListener (this);
        chooser->removeListener (this);
        watchButton.onClick = nullptr;
    }

    void onStateRestored()
    {
        auto watchDir = processor.getWatchDir();
        if (! watchDir.exists() || ! watchDir.isDirectory())
            return;
        addRecentsFrom (watchDir, true);
    }
};

AudioFilePlayerNode::AudioFilePlayerNode()
    : BaseProcessor (BusesProperties()
        .withOutput  ("Main",  AudioChannelSet::stereo(), true))
{
    addParameter (playing = new AudioParameterBool ("playing", "Playing", false));
    addParameter (slave   = new AudioParameterBool ("slave", "Slave", false));
    addParameter (volume  = new AudioParameterFloat ("volume", "Volume", -60.f, 12.f, 0.f));
    addParameter (looping = new AudioParameterBool ("loop", "Loop", false));

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
        reader->setLooping (*looping);
        player.setLooping (*looping);
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

        reader->setLooping (*looping);
        player.setLooping (*looping);
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

void AudioFilePlayerNode::setLooping (const bool shouldLoop)
{
    jassert (looping != nullptr);
    *looping = shouldLoop;
}

bool AudioFilePlayerNode::isLooping() const
{ 
    return *looping;
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
         .setProperty ("loop", (bool)*looping, nullptr)
         .setProperty ("midiStartStopContinue", midiStartStopContinue.get() == 1, nullptr);
    
    if (watchDir.exists())
        state.setProperty ("watchDir", watchDir.getFullPathName(), nullptr);

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
        *looping = (bool) state.getProperty ("loop", true);
        midiStartStopContinue.set ((bool) state.getProperty ("midiStartStopContinue", false) ? 1 : 0);
        if (state.hasProperty ("watchDir"))
        {
            auto watchPath = state["watchDir"].toString();
            if (File::isAbsolutePath (watchPath))
                watchDir = File (watchPath);
        }
        restoredState();
    }
}

void AudioFilePlayerNode::parameterValueChanged (int parameter, float newValue)
{
    ignoreUnused (newValue);

    switch (parameter)
    {
        case Playing:
        {
            if (*playing)
                player.start();
            else
                player.stop();
        } break;
        
        case Slave: {
            // noop
        } break;

        case Volume:
        {
            player.setGain (Decibels::decibelsToGain (volume->get(), volume->range.start));
        } break;

        case Looping:
        {
            if (reader != nullptr)
            {
                player.setLooping (*looping);
                reader->setLooping (*looping);
            }
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
