
#include "gui/widgets/MidiBlinker.h"
#include "gui/LookAndFeel.h"

namespace Element {

MidiBlinker::MidiBlinker()
{ 
    setTooltip ("Blinks when MIDI is sent or received from MIDI devices.");
}

MidiBlinker::~MidiBlinker() { }

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

}