// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"
#include "ui/buttons.hpp"
#include <element/audioengine.hpp>
#include <element/session.hpp>

namespace element {

class BarLabel;
class TransportBar : public Component,
                     public Button::Listener,
                     private Timer
{
public:
    TransportBar();
    ~TransportBar();

    void setBeatTime (const float t);
    void updateWidth();
    void stabilize();

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;

private:
    SessionPtr session;
    AudioEnginePtr engine;
    Transport::MonitorPtr monitor;

    std::unique_ptr<SettingButton> play;
    std::unique_ptr<SettingButton> stop;
    std::unique_ptr<SettingButton> record;
    std::unique_ptr<DragableIntLabel> barLabel;
    std::unique_ptr<DragableIntLabel> beatLabel;
    std::unique_ptr<DragableIntLabel> subLabel;

    friend class BarLabel;
    friend class Timer;
    void timerCallback() override;

    bool checkForMonitor();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

} // namespace element
