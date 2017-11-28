/*
    This file is part of Element
    Copy right 2018 Kushview, LLC.  All rights reserved
*/

#pragma once

#include "ElementApp.h"
#include "gui/Buttons.h"
#include "engine/AudioEngine.h"
#include "session/Session.h"

namespace Element {

class TransportBar  : public Component,
                      public Button::Listener,
                      private Timer
{
public:
    TransportBar ();
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

    ScopedPointer<SettingButton> play;
    ScopedPointer<SettingButton> stop;
    ScopedPointer<SettingButton> record;
    ScopedPointer<DragableIntLabel> barLabel;
    ScopedPointer<DragableIntLabel> beatLabel;
    ScopedPointer<DragableIntLabel> subLabel;
    
    friend class Timer;
    void timerCallback() override;
    
    bool checkForMonitor();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

} /* namespace element */
