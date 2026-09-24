// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "ElementApp.h"
#include "ui/buttons.hpp"
#include <element/audioengine.hpp>
#include <element/session.hpp>
#include <element/transport.hpp>

namespace element {

class BarLabel;
class MappingService;
class TransportBar : public Component,
                     private Button::Listener,
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

private:
    SessionPtr session;
    AudioEnginePtr engine;
    Transport::MonitorPtr monitor;

    std::unique_ptr<SettingButton> play, stop, record, toZero;
    std::unique_ptr<DragableIntLabel> barLabel, beatLabel, subLabel;

    friend class BarLabel;
    friend class Timer;

    void buttonClicked (Button* buttonThatWasClicked) override;
    void timerCallback() override;

    bool checkForMonitor();
    MappingService* findMappingService();

    /** The transport action a button performs, or nullopt for other buttons. */
    std::optional<TransportAction> actionFor (Button* button) const;

    /** Right-click menu to learn or clear the MIDI mapping of a button. */
    void showLearnMenu (SettingButton& button, TransportAction action);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

} // namespace element
