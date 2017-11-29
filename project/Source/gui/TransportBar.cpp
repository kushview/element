
#include "session/Session.h"
#include "gui/GuiCommon.h"
#include "gui/TransportBar.h"

namespace Element {

class BarLabel : public DragableIntLabel
{
public:
    BarLabel (TransportBar& t) : owner(t)
    {
        setDragable (false);
    }
    
    void settingLabelDoubleClicked() override
    {
        if (auto e = owner.engine)
            e->seekToAudioFrame (0);
    }
    
    TransportBar& owner;
};

class BeatLabel : public DragableIntLabel {
public:
    BeatLabel()
    {
        setDragable (false);
    }
};

class SubBeatLabel : public DragableIntLabel {
public:
    SubBeatLabel()
    {
        setDragable (false);
    }
};
    
TransportBar::TransportBar ()
{
    addAndMakeVisible (play = new SettingButton ("play"));
    play->setButtonText (TRANS("Play"));
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    play->setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen);

    addAndMakeVisible (stop = new SettingButton ("stop"));
    stop->setButtonText (TRANS("Stop"));
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    addAndMakeVisible (record = new SettingButton ("record"));
    record->setButtonText (TRANS("Rec"));
    record->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    record->addListener (this);
    record->setColour (SettingButton::backgroundOnColourId, Colours::red);

    addAndMakeVisible (barLabel = new BarLabel (*this));
    barLabel->setName ("barLabel");

    addAndMakeVisible (beatLabel = new BeatLabel());
    beatLabel->setName ("beatLabel");

    addAndMakeVisible (subLabel = new SubBeatLabel());
    subLabel->setName ("subLabel");

    setBeatTime (0.f);
    setSize (260, 16);
    updateWidth();
    
    startTimer (44);
}

TransportBar::~TransportBar()
{
    play = nullptr;
    stop = nullptr;
    record = nullptr;
    barLabel = nullptr;
    beatLabel = nullptr;
    subLabel = nullptr;
}

bool TransportBar::checkForMonitor()
{
    if (nullptr == monitor)
    {
        if (auto* w = ViewHelpers::getGlobals (this))
        {
            engine  = w->getAudioEngine();
            monitor = engine->getTransportMonitor();
            session = w->getSession();
        }
    }
    
    return monitor != nullptr;
}

void TransportBar::timerCallback()
{
    if (! checkForMonitor())
        return;

    if (play->getToggleState() != monitor->playing.get())
        play->setToggleState (monitor->playing.get(), dontSendNotification);
    if (record->getToggleState() != monitor->recording.get())
        record->setToggleState (monitor->recording.get(), dontSendNotification);

    stabilize();
}

void TransportBar::paint (Graphics& g)
{
    
}

void TransportBar::resized()
{
    play->setBounds (80, 0, 32, 16);
    stop->setBounds (115, 0, 32, 16);
    record->setBounds (150, 0, 32, 16);
    
    barLabel->setBounds (0, 0, 24, 16);
    beatLabel->setBounds (26, 0, 24, 16);
    subLabel->setBounds (52, 0, 24, 16);
}

void TransportBar::buttonClicked (Button* buttonThatWasClicked)
{
    if (! checkForMonitor())
        return;
    
    if (buttonThatWasClicked == play)
    {
        if (monitor->playing.get())
            engine->seekToAudioFrame (0);
        else
            engine->setPlaying (true);
    }
    else if (buttonThatWasClicked == stop)
    {
        if (! monitor->playing.get())
            engine->seekToAudioFrame (0);
        else
            engine->setPlaying (false);
    }
    else if (buttonThatWasClicked == record)
    {
        engine->setRecording (! monitor->recording.get());
    }
}

void TransportBar::setBeatTime (const float t)
{
    
}

void TransportBar::stabilize()
{
    if (checkForMonitor())
    {
        int bars = 0, beats = 0, sub = 0;
        monitor->getBarsAndBeats (bars, beats, sub);
        barLabel->tempoValue  = bars + 1;
        beatLabel->tempoValue = beats + 1;
        subLabel->tempoValue  = sub + 1;
        
        for (auto* c : { barLabel.get(), beatLabel.get(), subLabel.get() })
            c->repaint();
    }
}

void TransportBar::updateWidth()
{
    setSize (record->getRight(), getHeight());
}

} /* namespace element */
