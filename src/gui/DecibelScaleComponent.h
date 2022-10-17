/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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
//==============================================================================
/**
    Creates a decibel scale using iec lin 2 db scale
*/
class DecibelScaleComponent : public Component
{
public:
    enum DecibelLevels
    {
        LevelOver = 0,
        Level0dB = 1,
        Level3dB = 2,
        Level6dB = 3,
        Level10dB = 4,
        LevelCount = 5
    };

    enum ColourIds
    {
        markerColourId = 0x11112222
    };

    DecibelScaleComponent();
    ~DecibelScaleComponent();

    int iecScale (const float dB) const;
    int iecLevel (const int index) const;

    /** @internal */
    void paint (Graphics& g);
    /** @internal */
    void resized();

protected:
    void drawLabel (Graphics& g, const int y, const String& label);

private:
    Font font;
    float scale;
    int lastY;
    int levels[LevelCount];
};

} // namespace element
