#pragma once

#include "JuceHeader.h"

namespace Element {

struct Spinner  : public Component,
                  private Timer
{
    Spinner()                       { startTimer (1000 / 50); }
    void timerCallback() override   { repaint(); }
    void paint (Graphics& g) override
    {
        getLookAndFeel().drawSpinningWaitAnimation (
            g, Colours::darkgrey, 0, 0, getWidth(), getHeight());
    }
};

}
