
#include "gui/Buttons.h"

namespace Element {

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
        Icon icon (path, getTextColour());
        Rectangle<float> r { 0.0, 0.0, (float)getWidth(), (float)getHeight() };

        icon.draw (g, r.reduced (2), false);
    }

    if (icon.isNull() || !icon.isValid())
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
        const Rectangle<float> area (0.f, 0.f, getWidth(), getHeight());
        g.drawImage (icon, area, RectanglePlacement::onlyReduceInSize);
    }

    g.setColour (LookAndFeel::widgetBackgroundColor.brighter().brighter());
    g.drawRect (0, 0, getWidth(), getHeight());
}

}
