// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/session.hpp>

#include "services/mappingservice.hpp"
#include "ui/guicommon.hpp"
#include "ui/transportbar.hpp"
#include "ui/viewhelpers.hpp"

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
            e->performTransportAction (TransportAction::SeekZero);
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
    play = std::make_unique<PlayButton>();
    addAndMakeVisible (play.get());
    play->setPath (getIcons().fasPlay, 4.4f);
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    play->setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen);

    stop = std::make_unique<StopButton>();
    addAndMakeVisible (stop.get());
    stop->setPath (getIcons().fasStop, 4.4f);
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    record = std::make_unique<RecordButton>();
    addAndMakeVisible (record.get());
    record->setPath (getIcons().fasCircle, 4.4f);
    record->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    record->addListener (this);
    record->setColour (SettingButton::backgroundOnColourId, Colours::red);

    toZero = std::make_unique<SeekZeroButton>();
    addAndMakeVisible (toZero.get());
    auto toZeroPath = getIcons().fasChevronRight;
    toZeroPath.applyTransform (AffineTransform().rotated (juce::MathConstants<float>::pi));
    toZero->setPath (toZeroPath, 4.4f);
    toZero->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    toZero->addListener (this);

    for (auto* button : { play.get(), stop.get(), record.get(), toZero.get() })
        button->onContextMenu = [this, button] {
            if (auto action = actionFor (button))
                showLearnMenu (*button, *action);
        };

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

std::optional<TransportAction> TransportBar::actionFor (Button* button) const
{
    if (button == play.get())
        return TransportAction::Play;
    if (button == stop.get())
        return TransportAction::Stop;
    if (button == record.get())
        return TransportAction::Record;
    if (button == toZero.get())
        return TransportAction::SeekZero;
    return std::nullopt;
}

MappingService* TransportBar::findMappingService()
{
    if (auto* cc = ViewHelpers::findContentComponent (this))
        return cc->services().find<MappingService>();
    return nullptr;
}

void TransportBar::buttonClicked (Button* buttonThatWasClicked)
{
    const auto action = actionFor (buttonThatWasClicked);
    if (! action || ! checkForMonitor())
        return;

    // In MIDI-map mode a click arms capture for this button ("map, then click
    // the thing to map") instead of driving the transport; the next MIDI
    // event binds it.
    if (auto* maps = findMappingService(); maps != nullptr && maps->isLearning())
        maps->learnTransport (*action);
    else
        engine->performTransportAction (*action);
}

void TransportBar::showLearnMenu (SettingButton& button, TransportAction action)
{
    auto* maps = findMappingService();
    if (maps == nullptr)
        return;

    ViewHelpers::showMidiLearnMenu (
        button,
        TRANS ("MIDI Learn ") + getTransportActionName (action),
        maps->hasTransportMapping (action),
        maps->getTransportMappingDescription (action),
        [this, action] { if (auto* svc = findMappingService()) svc->learnTransport (action); },
        [this, action] { if (auto* svc = findMappingService()) svc->clearTransportMapping (action); });
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
