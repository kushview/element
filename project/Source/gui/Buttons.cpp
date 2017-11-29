
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
    
    String text = getButtonText();
    
    if (text.isEmpty() && getClickingTogglesState())
        text = (getToggleState()) ? yes : no;
    
    g.setFont (12.f);
    g.setColour (getTextColour());
    g.drawText (text, getLocalBounds(), Justification::centred);
    g.setColour (LookAndFeel::widgetBackgroundColor.brighter().brighter());
    g.drawRect (0, 0, getWidth(), getHeight());
}

}
