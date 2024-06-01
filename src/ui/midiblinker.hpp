// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce.hpp>
using namespace juce; // FIXME;

namespace element {

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

    void setInputOutputVisibility (bool in, bool out);

    void paint (Graphics&) override;
    void resized() override;

private:
    int holdMillis = 100;
    bool haveInput = false;
    bool haveOutput = false;
    bool showInput = true;
    bool showOutput = true;
    friend class Timer;
    void timerCallback() override;
};

} // namespace element
