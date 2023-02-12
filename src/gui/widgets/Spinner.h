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

#include <element/juce.hpp>

namespace element {

struct Spinner : public Component,
                 private Timer
{
    Spinner() { startTimer (1000 / 50); }
    void timerCallback() override { repaint(); }
    void paint (Graphics& g) override
    {
        getLookAndFeel().drawSpinningWaitAnimation (
            g, Colours::darkgrey, 0, 0, getWidth(), getHeight());
    }
};

} // namespace element
