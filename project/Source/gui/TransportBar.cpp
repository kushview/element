
#include "session/Session.h"
#include "gui/GuiCommon.h"
#include "gui/TransportBar.h"

namespace Element {

TransportBar::TransportBar ()
{
    addAndMakeVisible (play = new SettingButton ("play"));
    play->setButtonText (TRANS("Play"));
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);

    addAndMakeVisible (stop = new SettingButton ("stop"));
    stop->setButtonText (TRANS("Stop"));
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    addAndMakeVisible (record = new SettingButton ("record"));
    record->setButtonText (TRANS("Rec"));
    record->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    record->addListener (this);
    record->setColour (TextButton::buttonOnColourId, Colours::red);
    record->setEnabled (false); // TODO: recording

    addAndMakeVisible (barLabel = new DragableIntLabel());
    barLabel->setName ("barLabel");

    addAndMakeVisible (beatLabel = new DragableIntLabel());
    beatLabel->setName ("beatLabel");

    addAndMakeVisible (subLabel = new DragableIntLabel());
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
    
    setBeatTime (monitor->getPositionBeats());
}

void TransportBar::paint (Graphics& g)
{
    
}

void TransportBar::resized()
{
    play->setBounds (150, 0, 32, 16);
    stop->setBounds (115, 0, 32, 16);
    record->setBounds (80, 0, 32, 16);
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
        engine->setPlaying (! monitor->playing.get());
    }
    else if (buttonThatWasClicked == stop)
    {
        engine->setPlaying (false);
    }
    else if (buttonThatWasClicked == record)
    {

    }
}

void TransportBar::setBeatTime (const float t)
{
    const float beatsPerBar = 4.f;
    const int bars = std::floor (t / beatsPerBar);
    const int beats = std::floor (t);
    
    barLabel->tempoValue = bars + 1;
    beatLabel->tempoValue = (beats % 4) + 1;
    subLabel->tempoValue = 1;
    
    for (auto* c : { barLabel.get(), beatLabel.get(), subLabel.get() })
        c->repaint();
}

void TransportBar::stabilize()
{

}

void TransportBar::updateWidth()
{
    setSize (play->getRight(), getHeight());
}

} /* namespace element */
