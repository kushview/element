// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ui/midiblinker.hpp"
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

    if (showInput && showOutput)
    {
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
    else if (! showInput && ! showOutput)
    {
        g.fillAll (bgc);
        g.setColour (olc);
        g.drawRect (getLocalBounds());
    }
    else
    {
        auto r3 = getLocalBounds().reduced (1);
        auto c = (showInput && haveInput) || (showOutput && haveOutput)
                     ? Colors::toggleGreen
                     : bgc;
        g.setColour (c);
        g.fillRect (r3);
        g.setColour (olc);
        g.drawRect (r3);
    }
}

void MidiBlinker::setInputOutputVisibility (bool in, bool out)
{
    showInput = in;
    showOutput = out;
    repaint();
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