/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include <element/session.hpp>
#include "gui/GuiCommon.h"
#include "gui/TransportBar.h"

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
    addAndMakeVisible (play = new SettingButton());
    play->setPath (getIcons().fasPlay, 4.4);
    play->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    play->addListener (this);
    play->setColour (TextButton::buttonOnColourId, Colours::chartreuse);
    play->setColour (SettingButton::backgroundOnColourId, Colors::toggleGreen);

    addAndMakeVisible (stop = new SettingButton());
    stop->setPath (getIcons().fasStop, 4.4);
    stop->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight | Button::ConnectedOnTop | Button::ConnectedOnBottom);
    stop->addListener (this);

    addAndMakeVisible (record = new SettingButton());
    record->setPath (getIcons().fasCircle, 4.4);
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
            engine = w->getAudioEngine();
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
    play->setBounds (80, 0, 20, 16);
    stop->setBounds (102, 0, 20, 16);
    record->setBounds (124, 0, 20, 16);

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
        barLabel->tempoValue = bars + 1;
        beatLabel->tempoValue = beats + 1;
        subLabel->tempoValue = sub + 1;
        for (auto* c : { barLabel.get(), beatLabel.get(), subLabel.get() })
            c->repaint();
    }
}

void TransportBar::updateWidth()
{
    setSize (record->getRight(), getHeight());
}

} // namespace element
