
#include "LookAndFeel.h"

namespace Element {

const Colour Colors::elemental      = LookAndFeel_KV1::elementBlue;
const Colour Colors::toggleBlue     = Colour (0xff33aaf9);
const Colour Colors::toggleGreen    = Colour (0xff92e75e);
const Colour Colors::toggleOrange   = Colour (0xfffaa63a);

void LookAndFeel::drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                             bool isMouseOver, bool isMouseDown,
                                             ConcertinaPanel& panel, Component& comp)
{
    g.setColour (Colour (0xff323232));
    Rectangle<int> r (area.withSizeKeepingCentre (area.getWidth(), area.getHeight() - 2));
    g.fillRect (r);
}

// MARK: Property Panel

void LookAndFeel::drawPropertyPanelSectionHeader (Graphics& g, const String& name,
                                                  bool isOpen, int width, int height)
{
    String text = isOpen ? " - " : " + "; text << name;
    g.setColour (isOpen ? LookAndFeel::textBoldColor : LookAndFeel::textColor);
    g.drawText (text, 0, 0, width, height, Justification::centredLeft);
}
    
void LookAndFeel::drawPropertyComponentBackground (Graphics& g, int width, int height,
                                                   PropertyComponent& pc)
{
    LookAndFeel_KV1::drawPropertyComponentBackground (g, width, height, pc);
}
    
void LookAndFeel::drawPropertyComponentLabel (Graphics& g, int width, int height,
                                              PropertyComponent& pc)
{
    LookAndFeel_KV1::drawPropertyComponentLabel (g, width, height, pc);
}
    
Rectangle<int> LookAndFeel::getPropertyComponentContentPosition (PropertyComponent& pc)
{
    return LookAndFeel_KV1::getPropertyComponentContentPosition (pc);
}
    
}
