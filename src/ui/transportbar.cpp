// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/session.hpp>

#include "ui/guicommon.hpp"
#include "ui/transportbar.hpp"

namespace element {

class BarLabel : public DragableIntLabel
{
public:
    BarLabel (TransportBar& t) : owner (t)
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

class BeatLabel : public DragableIntLabel
{
public:
    BeatLabel()
    {
        setDragable (false);
    }
};

class SubBeatLabel : public DragableIntLabel
{
public:
    SubBeatLabel()
    {
        setDragable (false);
    }
};

TransportBar::TransportBar()
{
    play = std::make_unique<SettingButton>();
    addAndMakeVisible (play.get());
    play->setPath (getIcons().fasPlay, 4.4f);
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    play->setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen);

    stop = std::make_unique<SettingButton>();
    addAndMakeVisible (stop.get());
    stop->setPath (getIcons().fasStop, 4.4f);
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    record = std::make_unique<SettingButton>();
    addAndMakeVisible (record.get());
    record->setPath (getIcons().fasCircle, 4.4f);
    record->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    record->addListener (this);
    record->setColour (SettingButton::backgroundOnColourId, Colours::red);

    toZero = std::make_unique<SettingButton>();
    addAndMakeVisible (toZero.get());
    auto toZeroPath = getIcons().fasChevronRight;
    toZeroPath.applyTransform (AffineTransform().rotated (juce::MathConstants<float>::pi));
    toZero->setPath (toZeroPath, 4.4f);
    toZero->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    toZero->addListener (this);

    barLabel = std::make_unique<BarLabel> (*this);
    addAndMakeVisible (barLabel.get());
    barLabel->setName ("barLabel");

    beatLabel = std::make_unique<BeatLabel>();
    addAndMakeVisible (beatLabel.get());
    beatLabel->setName ("beatLabel");

    subLabel = std::make_unique<SubBeatLabel>();
    addAndMakeVisible (subLabel.get());
    subLabel->setName ("subLabel");

    setBeatTime (0.f);
    setSize (280, 16);
    updateWidth();

    startTimer (88);
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
            engine = w->audio();
            monitor = engine->getTransportMonitor();
            session = w->session();
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
    play->setBounds (80, 0, 20, 16);
    stop->setBounds (102, 0, 20, 16);
    record->setBounds (124, 0, 20, 16);
    toZero->setBounds (146, 0, 20, 16);

    barLabel->setBounds (0, 0, 24, 16);
    beatLabel->setBounds (26, 0, 24, 16);
    subLabel->setBounds (52, 0, 24, 16);
}

void TransportBar::buttonClicked (Button* buttonThatWasClicked)
{
    if (! checkForMonitor())
        return;

    if (buttonThatWasClicked == play.get())
    {
        if (monitor->playing.get())
            engine->seekToAudioFrame (0);
        else
            engine->setPlaying (true);
    }
    else if (buttonThatWasClicked == toZero.get())
    {
        engine->seekToAudioFrame (0);
    }
    else if (buttonThatWasClicked == stop.get())
    {
        if (! monitor->playing.get())
            engine->seekToAudioFrame (0);
        else
            engine->setPlaying (false);
    }
    else if (buttonThatWasClicked == record.get())
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
        // std::clog << "pos beats monitor: " << monitor->getPositionBeats() << std::endl;

        int bars = 0, beats = 0, sub = 0;
        monitor->getBarsAndBeats (bars, beats, sub);
        barLabel->tempoValue = bars + 1;
        beatLabel->tempoValue = beats + 1;
        subLabel->tempoValue = sub + 1;
        for (auto* c : { barLabel.get(), beatLabel.get(), subLabel.get() })
            c->repaint();
    }
}

void TransportBar::updateWidth()
{
    setSize (toZero->getRight(), getHeight());
}

} // namespace element
