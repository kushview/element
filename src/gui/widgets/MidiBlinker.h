#pragma once

#include "JuceHeader.h"

namespace Element {

class MidiBlinker : public Component,
                    public SettableTooltipClient,
                    private Timer
{
public:
    enum ColourIds
    {
        backgroundColourId = 0x90001000,
        outlineColourId 
    };

    MidiBlinker();
    virtual ~MidiBlinker();

    void triggerReceived();
    void triggerSent();

    void paint (Graphics&) override;
    void resized() override;

private:
    int holdMillis = 100;
    bool haveInput = false;
    bool haveOutput = false;
    friend class Timer;
    void timerCallback() override;

};

}
