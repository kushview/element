// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ui/buttons.hpp"

namespace element {

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
    icon.colour = isEnabled() ? Colors::textColor : Colors::textColor.darker();
    icon.draw (g, bounds.reduced (iconReduceSize), false);
}

void SettingButton::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    const bool isOn = getToggleState();
    //    Colour fill = isOn ? Colors::toggleOrange : Colors::widgetBackgroundColor.brighter();

    Colour fill = findColour (isOn ? backgroundOnColourId : backgroundColourId);
    if (isOn)
    {
    }
    else if (isButtonDown)
    {
        fill = fill.darker (0.20f);
    }
    else if (isMouseOverButton)
    {
        fill = fill.brighter (0.12f);
    }

    g.fillAll (fill);

    if (! path.isEmpty())
    {
        Icon i (path, getTextColour().brighter (0.15f));
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
        g.drawImage (icon, area.reduced (2).toFloat(), RectanglePlacement::onlyReduceInSize);
    }

    g.setColour (Colors::widgetBackgroundColor.brighter().brighter());
    g.drawRect (0, 0, getWidth(), getHeight());
}

} // namespace element
