
#include "engine/AudioMixerProcessor.h"
#include "gui/HorizontalListBox.h"

namespace Element {

class AudioMixerComponent : public Component
{
public:
    AudioMixerComponent() { 
        addAndMakeVisible (channels);
    }

    void resized() override
    {
        channels.setBounds (getLocalBounds());
    }

private:
    class ChannelStrip : public Component
    {
    public:
        ChannelStrip()
        { 
            
        }
        
        ~ChannelStrip() { }

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::orange);
        }

       
        void refresh (int, bool) { }
    };

    class ChannelList : public HorizontalListBox,
                        public ListBoxModel
    {
    public:
        ChannelList()
        {
            setModel (this);
            setRowHeight (64);
        }

        ~ChannelList()
        {
            setModel (nullptr);
        }

        int getNumRows() override { return 4; }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override { }
        
        Component* refreshComponentForRow (int rowNumber, bool isRowSelected,
                                           Component* existingComponentToUpdate) override
        {
            ChannelStrip* strip = dynamic_cast<ChannelStrip*> (existingComponentToUpdate);
            if (nullptr == strip) strip = new ChannelStrip();
            if (strip) strip->refresh (rowNumber, isRowSelected);
            return strip;
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
    };

    ChannelList channels;
};

class AudioMixerEditor : public AudioProcessorEditor
{
public:
    AudioMixerEditor (AudioMixerProcessor& p) 
        : AudioProcessorEditor (&p)
    {
        // setOpaque (false);
        setName ("AudioMixerEditor");
        addAndMakeVisible (mixer);
        setSize (640, 360);
    }

    ~AudioMixerEditor() noexcept { }

    void paint (Graphics& g) override 
    {
        g.fillAll (Colours::black);
    }

    void resized() override
    {
        mixer.setBounds (getLocalBounds().reduced(2));
    }

private:
    AudioMixerComponent mixer;
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
        wasAdded = true; //input->setCurrentLayout (AudioChannelSet::stereo());

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
    return new AudioMixerEditor (*this);
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
