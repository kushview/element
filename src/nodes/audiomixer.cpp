// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nodes/audiomixer.hpp"
#include "ui/horizontallistbox.hpp"
#include <element/ui/style.hpp>
#include <element/ui/simplemeter.hpp>

#define EL_FADER_MIN_DB -90.0
#define EL_FADER_MAX_DB 12.0

namespace element {

typedef AudioMixerProcessor::MonitorPtr MonitorPtr;

class AudioMixerEditor : public AudioProcessorEditor,
                         private Timer
{
public:
    AudioMixerEditor (AudioMixerProcessor& p)
        : AudioProcessorEditor (&p),
          owner (p),
          channels (*this)
    {
        setName ("AudioMixerEditor");
        addAndMakeVisible (channels);
        setSize (330, 210);
        startTimerHz (24);
    }

    ~AudioMixerEditor() noexcept {}

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (2);
        r.setHeight (jmax (100, r.getHeight()));
        if (masterStrip)
        {
            masterStrip->setBounds (r.removeFromRight (64));
            r.removeFromRight (2);
        }

        channels.setBounds (r);

        if (auto* hs = channels.getHorizontalScrollBar())
        {
            if (masterStrip && hs->isShowing())
            {
                masterStrip->setBounds (masterStrip->getBoundsInParent().withHeight (
                    masterStrip->getHeight() - hs->getHeight()));
            }
        }
    }

    void rebuildTracks()
    {
        monitors.clearQuick();
        for (int i = 0; i < owner.getNumTracks(); ++i)
            monitors.add (owner.getMonitor (i));
        channels.updateContent();

        masterMonitor = owner.getMonitor();
        masterStrip = std::make_unique<ChannelStrip> (*this, masterMonitor);
        addAndMakeVisible (masterStrip.get());

        resized();
    }

private:
    AudioMixerProcessor& owner;
    class ChannelStrip : public Component,
                         public Button::Listener,
                         public Slider::Listener
    {
    public:
        ChannelStrip (AudioMixerEditor& ed, AudioMixerProcessor::Monitor* mon)
            : editor (ed), monitor (mon), meter (mon->getNumChannels())
        {
            addAndMakeVisible (fader);
            fader.setSliderStyle (Slider::LinearBarVertical);
            fader.setTextBoxStyle (Slider::NoTextBox, true, 1, 1);
            fader.setRange (EL_FADER_MIN_DB, EL_FADER_MAX_DB, 0.001);
            fader.setValue (0.f, dontSendNotification);
            fader.setSkewFactor (2);
            fader.setDoubleClickReturnValue (true, 0.0);
            fader.addListener (this);

            addAndMakeVisible (meter);

            addAndMakeVisible (name);
            name.setFont (name.getFont().withHeight (14));
            name.setJustificationType (Justification::centred);
            setTrackName (monitor->getTrackId() >= 0 ? "Track " + String (monitor->getTrackId() + 1)
                                                     : "Master");

            addAndMakeVisible (mute);
            mute.setColour (TextButton::buttonOnColourId, Colors::toggleRed);
            mute.setButtonText ("M");
            mute.addListener (this);

            addAndMakeVisible (volume);
            volume.setFont (volume.getFont().withHeight (12));
            volume.setJustificationType (Justification::centred);
            stabilizeContent();
            resized();

            editor.strips.add (this);
        }

        ~ChannelStrip() { editor.strips.removeFirstMatchingValue (this); }

        void setTrackName (const String& n)
        {
            name.setText (n, dontSendNotification);
        }

        void setMonitor (MonitorPtr ptr)
        {
            if (ptr == monitor)
                return;
            monitor = ptr;
        }

        void paint (Graphics& g) override
        {
            g.fillAll (Colors::widgetBackgroundColor);
        }

        void resized() override
        {
            auto r = getLocalBounds();
            name.setBounds (r.removeFromTop (18));

            volume.setBounds (r.removeFromBottom (18));
            auto r2 = r.removeFromBottom (18);
            mute.setBounds (r2.removeFromRight (getWidth() / 3));

            fader.setBounds (r.removeFromRight (getWidth() / 2));
            meter.setBounds (r);
        }

        void buttonClicked (Button* button) override
        {
            if (button == &mute)
            {
                monitor->requestMute (! mute.getToggleState());
            }
        }

        void sliderValueChanged (Slider* s) override
        {
            if (s == &fader)
            {
                monitor->requestVolume (s->getValue());
                updateLabels();
            }
        }

        int getNumChannels() const { return (nullptr != monitor) ? monitor->getNumChannels()
                                                                 : 0; }

    private:
        friend class AudioMixerEditor;

        AudioMixerEditor& editor;
        AudioMixerProcessor::MonitorPtr monitor;
        Slider fader;
        SimpleMeter meter;
        TextButton mute;
        Label name;
        Label volume;

        void updateLabels()
        {
            String voltxt = String (fader.getValue(), 2);
            voltxt << "dB";
            volume.setText (voltxt, dontSendNotification);
        }

        void stabilizeContent()
        {
            const double dB = (double) Decibels::gainToDecibels (monitor->getGain(), (float) EL_FADER_MIN_DB);
            if (fader.getValue() != dB)
            {
                fader.setValue (dB, dontSendNotification);
                updateLabels();
            }

            mute.setToggleState (monitor->isMuted(), dontSendNotification);
        }

        void processMeter()
        {
            for (int i = 0; i < monitor->getNumChannels(); ++i)
                meter.setValue (i, monitor->getLevel (i));
            meter.repaint();
        }
    };

    typedef ReferenceCountedArray<AudioMixerProcessor::Monitor> MonitorList;

    class ChannelList : public HorizontalListBox,
                        public ListBoxModel
    {
    public:
        ChannelList (AudioMixerEditor& o)
            : owner (o)
        {
            setModel (this);
            setRowHeight (64);
        }

        ~ChannelList()
        {
            setModel (nullptr);
        }

        int getNumRows() override { return owner.monitors.size(); }

        void paintListBoxItem (int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override {}

        Component* refreshComponentForRow (int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
        {
            if (auto monitor = owner.monitors[rowNumber])
            {
                ChannelStrip* strip = dynamic_cast<ChannelStrip*> (existingComponentToUpdate);
                if (nullptr == strip)
                    strip = new ChannelStrip (owner, monitor);
                if (strip)
                    strip->setMonitor (monitor);
                return strip;
            }
            else
            {
                // noop: no monitor for strip
            }

            return nullptr;
        }

#if 0
        /** This can be overridden to react to the user clicking on a row.
            @see listBoxItemDoubleClicked
        */
        virtual void listBoxItemClicked (int row, const MouseEvent&);

        /** This can be overridden to react to the user double-clicking on a row.
            @see listBoxItemClicked
        */
        virtual void listBoxItemDoubleClicked (int row, const MouseEvent&);

        /** This can be overridden to react to the user clicking on a part of the list where
            there are no rows.
            @see listBoxItemClicked
        */
        virtual void backgroundClicked (const MouseEvent&);

        /** Override this to be informed when rows are selected or deselected.

            This will be called whenever a row is selected or deselected. If a range of
            rows is selected all at once, this will just be called once for that event.

            @param lastRowSelected      the last row that the user selected. If no
                                        rows are currently selected, this may be -1.
        */
        virtual void selectedRowsChanged (int lastRowSelected);

        /** Override this to be informed when the delete key is pressed.

            If no rows are selected when they press the key, this won't be called.

            @param lastRowSelected   the last row that had been selected when they pressed the
                                    key - if there are multiple selections, this might not be
                                    very useful
        */
        virtual void deleteKeyPressed (int lastRowSelected);

        /** Override this to be informed when the return key is pressed.

            If no rows are selected when they press the key, this won't be called.

            @param lastRowSelected   the last row that had been selected when they pressed the
                                    key - if there are multiple selections, this might not be
                                    very useful
        */
        virtual void returnKeyPressed (int lastRowSelected);

        /** Override this to be informed when the list is scrolled.

            This might be caused by the user moving the scrollbar, or by programmatic changes
            to the list position.
        */
        virtual void listWasScrolled();

        /** To allow rows from your list to be dragged-and-dropped, implement this method.

            If this returns a non-null variant then when the user drags a row, the listbox will
            try to find a DragAndDropContainer in its parent hierarchy, and will use it to trigger
            a drag-and-drop operation, using this string as the source description, with the listbox
            itself as the source component.

            @see DragAndDropContainer::startDragging
        */
        virtual var getDragSourceDescription (const SparseSet<int>& rowsToDescribe);

        /** You can override this to provide tool tips for specific rows.
            @see TooltipClient
        */
        virtual String getTooltipForRow (int row);

        /** You can override this to return a custom mouse cursor for each row. */
        virtual MouseCursor getMouseCursorForRow (int row);
#endif

    private:
        friend class AudioMixerEditor;
        AudioMixerEditor& owner;
    };

    friend class ChannelList;
    friend class ChannelStrip;

    ChannelList channels;
    Array<ChannelStrip*> strips;
    MonitorList monitors;
    std::unique_ptr<ChannelStrip> masterStrip;
    MonitorPtr masterMonitor;

    friend class Timer;
    void timerCallback() override
    {
        for (auto* const strip : strips)
        {
            strip->processMeter();
            strip->stabilizeContent();
        }
    }
};

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

AudioMixerProcessor::MonitorPtr AudioMixerProcessor::getMonitor (const int track) const
{
    if (track < 0)
        return masterMonitor;
    ScopedLock sl (getCallbackLock());
    if (! isPositiveAndBelow (track, tracks.size()))
        return nullptr;
    return tracks.getUnchecked (track)->monitor;
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
        wasAdded = true;

    if (wasAdded)
    {
        auto* const track = new Track();
        track->index = tracks.size();
        track->busIdx = input->getBusIndex();
        track->numInputs = input->getNumberOfChannels();
        track->numOutputs = input->getNumberOfChannels();
        track->lastGain = 1.0;
        track->gain = 1.0;
        track->mute = false;
        track->monitor = new Monitor (track->index, track->numOutputs);

        ScopedLock sl (getCallbackLock());
        tracks.add (track);
        numTracks = tracks.size();
    }
    else
    {
        DBG ("[element] AudioMixerProcessor: could not add new track");
    }
}

AudioProcessorEditor* AudioMixerProcessor::createEditor()
{
    auto* ed = new AudioMixerEditor (*this);
    ed->rebuildTracks();
    return ed;
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
    const int numChannels = audio.getNumChannels();
    tempBuffer.setSize (numChannels, numSamples, false, false, true);
    tempBuffer.clear (0, numSamples);

    for (int i = 0; i < tracks.size(); ++i)
    {
        auto* const track = tracks.getUnchecked (i);
        auto input (getBusBuffer<float> (audio, true, track->busIdx));
        auto& rms = track->monitor->rms;

        if (track->mute)
        {
            for (int c = 0; c < track->numInputs; ++c)
                rms.getReference (c).set (0.0);
        }
        else
        {
            for (int c = 0; c < track->numInputs; ++c)
            {
                rms.getReference (c).set (track->gain * input.getRMSLevel (c, 0, numSamples));
                tempBuffer.addFromWithRamp (c, 0, input.getReadPointer (c), numSamples, track->lastGain, track->gain);
            }
        }

        track->lastGain = track->gain;

        if (track->gain != track->monitor->nextGain.get())
            track->gain = track->monitor->nextGain.get();
        track->monitor->gain.set (track->gain);

        if (static_cast<int> (track->mute) != track->monitor->nextMute.get())
            track->mute = track->monitor->nextMute.get() > 0;
        track->monitor->muted.set (track->mute ? 1 : 0);
    }

    output.clear (0, numSamples);
    const float gain = Decibels::decibelsToGain ((float) *masterVolume, (float) EL_FADER_MIN_DB);
    if (! *masterMute)
        for (int c = 0; c < output.getNumChannels(); ++c)
            output.copyFromWithRamp (c, 0, tempBuffer.getReadPointer (c), numSamples, lastGain, gain);

    if (gain != masterMonitor->nextGain.get())
        *masterVolume = Decibels::gainToDecibels (masterMonitor->nextGain.get(), (float) EL_FADER_MIN_DB);
    if (static_cast<int> (*masterMute) != masterMonitor->nextMute.get())
        *masterMute = masterMonitor->nextMute.get() <= 0 ? false : true;

    masterMonitor->muted.set (*masterMute);
    masterMonitor->gain.set (gain);

    for (int i = 0; i < 2; ++i)
        masterMonitor->rms.getReference (i).set (
            output.getRMSLevel (i, 0, numSamples));

    lastGain = gain;
}

void AudioMixerProcessor::releaseResources()
{
    tempBuffer.setSize (1, 1, false, false, false);
}

bool AudioMixerProcessor::canApplyBusCountChange (bool isInput, bool isAdding, AudioProcessor::BusProperties& outProperties)
{
    if (isAdding && ! canAddBus (isInput))
        return false;
    if (! isAdding && ! canRemoveBus (isInput))
        return false;

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

void AudioMixerProcessor::setTrackGain (const int track, const float gain)
{
    if (! isPositiveAndBelow (track, numTracks))
        return;
    ScopedLock sl (getCallbackLock());
    tracks.getUnchecked (track)->gain = gain;
}

void AudioMixerProcessor::setTrackMuted (const int track, const bool mute)
{
    if (! isPositiveAndBelow (track, numTracks))
        return;
    ScopedLock sl (getCallbackLock());
    tracks.getUnchecked (track)->mute = mute;
}

bool AudioMixerProcessor::isTrackMuted (const int track) const
{
    if (! isPositiveAndBelow (track, numTracks))
        return false;
    ScopedLock sl (getCallbackLock());
    return tracks.getUnchecked (track)->mute;
}

float AudioMixerProcessor::getTrackGain (const int track) const
{
    if (! isPositiveAndBelow (track, numTracks))
        return 1.f;
    ScopedLock sl (getCallbackLock());
    return tracks.getUnchecked (track)->gain;
}

void AudioMixerProcessor::getStateInformation (juce::MemoryBlock& block)
{
    OwnedArray<Track> t;
    while (t.size() < numTracks)
        t.add (new Track());
    float volume = 0.0f;
    bool mute = false;
    {
        ScopedLock sl (getCallbackLock());
        for (int i = 0; i < numTracks; ++i)
            t.getUnchecked (i)->update (tracks.getUnchecked (i));
        volume = *masterVolume;
        mute = *masterMute;
    }

    ValueTree state ("audiomixer");
    state.setProperty (tags::volume, volume, 0)
        .setProperty ("mute", mute, 0);
    for (int i = 0; i < numTracks; ++i)
    {
        ValueTree trk ("track");
        auto* const track = t.getUnchecked (i);
        trk.setProperty ("index", track->index, 0)
            .setProperty ("busIdx", track->busIdx, 0)
            .setProperty ("numInputs", track->numInputs, 0)
            .setProperty ("numOutputs", track->numOutputs, 0)
            .setProperty ("gain", track->gain, 0)
            .setProperty ("mute", track->mute, 0);
        state.addChild (trk, -1, 0);
    }

    if (auto xml = state.createXml())
    {
        copyXmlToBinary (*xml, block);
    }
}

void AudioMixerProcessor::setStateInformation (const void* data, int size)
{
    ValueTree state;
    if (auto xml = getXmlFromBinary (data, size))
    {
        state = ValueTree::fromXml (*xml);
    }

    if (! state.isValid())
        return;

    Array<Track*> newTracks;
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const ValueTree trk (state.getChild (i));
        auto* const track = new Track();
        track->index = trk.getProperty ("index", i);
        track->busIdx = trk.getProperty ("busIdx", i);
        track->numInputs = trk.getProperty ("numInputs", 2);
        track->numOutputs = trk.getProperty ("numOutputs", 2);
        track->gain = trk.getProperty ("gain", 1.f);
        track->lastGain = track->gain;
        track->mute = (bool) trk.getProperty ("mute", false);

        track->monitor = new Monitor (track->index, track->numInputs);
        track->monitor->gain.set (track->gain);
        track->monitor->nextGain.set (track->gain);
        track->monitor->muted.set (track->mute ? 1 : 0);
        track->monitor->nextMute.set (track->mute ? 1 : 0);

        newTracks.add (track);
    }

    {
        ScopedLock sl (getCallbackLock());
        *masterVolume = (float) state.getProperty (tags::volume, 0.0);
        *masterMute = (bool) state.getProperty ("mute", false);
        masterMonitor->nextGain.set (Decibels::decibelsToGain ((float) *masterVolume, (float) EL_FADER_MIN_DB));
        masterMonitor->gain.set (masterMonitor->nextGain.get());
        masterMonitor->nextMute.set (*masterMute ? 1 : 0);
        masterMonitor->muted.set (masterMonitor->nextMute.get());
        tracks.swapWith (newTracks);
        numTracks = tracks.size();
    }

    for (auto* dt : newTracks)
        delete dt;
    newTracks.clear();
}

} // namespace element
