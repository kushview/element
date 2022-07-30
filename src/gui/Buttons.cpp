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

#include "gui/Buttons.h"

namespace Element {

IconButton::IconButton (const String& buttonName)
    : Button (buttonName) {}
IconButton::~IconButton() {}

void IconButton::setIcon (Icon newIcon, float reduceSize)
{
    icon = newIcon;
    repaint();
}

void IconButton::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    getLookAndFeel().drawButtonBackground (g, *this, findColour (getToggleState() ? TextButton::buttonOnColourId : TextButton::buttonColourId), isMouseOverButton, isButtonDown);
    Rectangle<float> bounds (0.f, 0.f, (float) jmin (getWidth(), getHeight()), (float) jmin (getWidth(), getHeight()));
    icon.colour = isEnabled() ? LookAndFeel::textColor : LookAndFeel::textColor.darker();
    icon.draw (g, bounds.reduced (iconReduceSize), false);
}

void SettingButton::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    const bool isOn = getToggleState();
    //    Colour fill = isOn ? Colors::toggleOrange : LookAndFeel::widgetBackgroundColor.brighter();

    Colour fill = findColour (isOn ? backgroundOnColourId : backgroundColourId);
    if (isOn)
    {
    }
    else if (isButtonDown)
    {
        fill = fill.darker (0.20);
    }
    else if (isMouseOverButton)
    {
        fill = fill.brighter (0.12);
    }

    g.fillAll (fill);

    if (! path.isEmpty())
    {
        Icon i (path, getTextColour().brighter (0.15));
        Rectangle<float> r { 0.0, 0.0, (float) getWidth(), (float) getHeight() };
        i.draw (g, r.reduced (pathReduction), false);
    }
    else if (icon.isNull() || ! icon.isValid())
    {
        String text = getButtonText();

        if (text.isEmpty() && getClickingTogglesState())
            text = (getToggleState()) ? yes : no;
        g.setFont (12.f);
        g.setColour (getTextColour());
        g.drawText (text, getLocalBounds(), Justification::centred);
    }
    else
    {
        const Rectangle<int> area (0, 0, getWidth(), getHeight());
        g.drawImage (icon, area.reduced (2).toFloat(), 
            RectanglePlacement::onlyReduceInSize);
    }

    g.setColour (LookAndFeel::widgetBackgroundColor.brighter().brighter());
    g.drawRect (0, 0, getWidth(), getHeight());
}

} // namespace Element
