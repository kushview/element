// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/ui/decibelscale.hpp>

using namespace juce;

namespace element {

DecibelScale::DecibelScale()
    : font (FontOptions (7.0f)), scale (0.0f), lastY (0)
{
    zeromem (levels, sizeof (int) * LevelCount);
    setColour (markerColourId, Colour (0xFFCCCCCC));
}

DecibelScale::~DecibelScale()
{
}

void DecibelScale::paint (Graphics& g)
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

void DecibelScale::resized()
{
    scale = 0.85f * getHeight();

    levels[Level0dB] = iecScale (0.0f);
    levels[Level3dB] = iecScale (-3.0f);
    levels[Level6dB] = iecScale (-6.0f);
    levels[Level10dB] = iecScale (-10.0f);
}

void DecibelScale::drawLabel (Graphics& g, const int y, const String& label)
{
    int iCurrY = getHeight() - y;
    int iWidth = getWidth();

    int iMidHeight = (int) (font.getHeight() * 0.5f);

    GlyphArrangement glyphs;
    glyphs.addLineOfText (font, label, 0, 0);
    if (glyphs.getBoundingBox (0, -1, true).getWidth() < iWidth - 5)
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

int DecibelScale::iecScale (const float dB) const
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

int DecibelScale::iecLevel (const int index) const
{
    return levels[index];
}

} // namespace element
