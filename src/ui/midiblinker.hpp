// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>

namespace element {

class MidiBlinker : public juce::Component,
                    public juce::SettableTooltipClient,
                    private juce::Timer
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

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int holdMillis = 100;
    bool haveInput = false;
    bool haveOutput = false;
    bool showInput = true;
    bool showOutput = true;
    friend class juce::Timer;
    void timerCallback() override;
};

} // namespace element
