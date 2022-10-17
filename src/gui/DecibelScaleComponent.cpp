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

#include "gui/DecibelScaleComponent.h"

namespace element {
DecibelScaleComponent::DecibelScaleComponent()
    : font (7.0f, Font::plain), scale (0.0f), lastY (0)
{
    zeromem (levels, sizeof (int) * LevelCount);
    setColour (markerColourId, Colour (0xFFCCCCCC));
}

DecibelScaleComponent::~DecibelScaleComponent()
{
}

void DecibelScaleComponent::paint (Graphics& g)
{
    g.setFont (font);
    g.setColour (findColour (markerColourId));

    lastY = 0;

    drawLabel (g, iecLevel (Level0dB), "0");
    drawLabel (g, iecLevel (Level3dB), "3");
    drawLabel (g, iecLevel (Level6dB), "6");
    drawLabel (g, iecLevel (Level10dB), "10");

    for (float dB = -20.0f; dB > -70.0f; dB -= 10.0f)
        drawLabel (g, iecScale (dB), String ((int) -dB));
}

void DecibelScaleComponent::resized()
{
    scale = 0.85f * getHeight();

    levels[Level0dB] = iecScale (0.0f);
    levels[Level3dB] = iecScale (-3.0f);
    levels[Level6dB] = iecScale (-6.0f);
    levels[Level10dB] = iecScale (-10.0f);
}

void DecibelScaleComponent::drawLabel (Graphics& g, const int y, const String& label)
{
    int iCurrY = getHeight() - y;
    int iWidth = getWidth();

    int iMidHeight = (int) (font.getHeight() * 0.5f);

    if (font.getStringWidth (label) < iWidth - 5)
    {
        g.drawLine (0, iCurrY, 2, iCurrY);
        g.drawLine (iWidth - 3, iCurrY, iWidth - 1, iCurrY);
    }

    if (iCurrY < iMidHeight || iCurrY > lastY + iMidHeight)
    {
        g.drawText (label,
                    2,
                    iCurrY - iMidHeight,
                    iWidth - 3,
                    (int) font.getHeight(),
                    Justification::centred,
                    false);

        lastY = iCurrY + 1;
    }
}

int DecibelScaleComponent::iecScale (const float dB) const
{
    float fDef = 1.0;

    if (dB < -70.0)
        fDef = 0.0;
    else if (dB < -60.0)
        fDef = (dB + 70.0) * 0.0025;
    else if (dB < -50.0)
        fDef = (dB + 60.0) * 0.005 + 0.025;
    else if (dB < -40.0)
        fDef = (dB + 50.0) * 0.0075 + 0.075;
    else if (dB < -30.0)
        fDef = (dB + 40.0) * 0.015 + 0.15;
    else if (dB < -20.0)
        fDef = (dB + 30.0) * 0.02 + 0.3;
    else // if (dB < 0.0)
        fDef = (dB + 20.0) * 0.025 + 0.5;

    return (int) (fDef * scale);
}

int DecibelScaleComponent::iecLevel (const int index) const
{
    return levels[index];
}

} // namespace element
