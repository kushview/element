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

#include "gui/widgets/MidiBlinker.h"
#include <element/ui/style.hpp>

namespace element {

MidiBlinker::MidiBlinker()
{
    setTooltip ("Blinks when MIDI is sent or received from MIDI devices.");
}

MidiBlinker::~MidiBlinker() {}

void MidiBlinker::paint (Graphics& g)
{
    const auto bgc = findColour (MidiBlinker::backgroundColourId);
    const auto olc = findColour (MidiBlinker::outlineColourId);

    auto r1 = getLocalBounds().removeFromTop ((getHeight() / 2) - 1);
    auto r2 = getLocalBounds().removeFromBottom ((getHeight() / 2) - 1);

    g.setColour (haveInput ? Colors::toggleGreen : bgc);
    g.fillRect (r1);

    g.setColour (haveOutput ? Colors::toggleGreen : bgc);
    g.fillRect (r2);

    g.setColour (olc);
    g.drawRect (r1);
    g.drawRect (r2);
}

void MidiBlinker::triggerReceived()
{
    haveInput = true;
    repaint();
    startTimer (holdMillis);
}

void MidiBlinker::triggerSent()
{
    haveOutput = true;
    repaint();
    startTimer (holdMillis);
}

void MidiBlinker::resized()
{
}

void MidiBlinker::timerCallback()
{
    haveInput = haveOutput = false;
    stopTimer();
    repaint();
}

} // namespace element