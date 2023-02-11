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

#pragma once

#include "ElementApp.h"
#include "gui/Buttons.h"
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

    ScopedPointer<SettingButton> play;
    ScopedPointer<SettingButton> stop;
    ScopedPointer<SettingButton> record;
    ScopedPointer<DragableIntLabel> barLabel;
    ScopedPointer<DragableIntLabel> beatLabel;
    ScopedPointer<DragableIntLabel> subLabel;

    friend class BarLabel;
    friend class Timer;
    void timerCallback() override;

    bool checkForMonitor();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportBar)
};

} // namespace element
