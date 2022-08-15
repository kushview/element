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

#include "JuceHeader.h"

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

    void paint (Graphics&) override;
    void resized() override;

private:
    int holdMillis = 100;
    bool haveInput = false;
    bool haveOutput = false;
    friend class Timer;
    void timerCallback() override;
};

} // namespace element
