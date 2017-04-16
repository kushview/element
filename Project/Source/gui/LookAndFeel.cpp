
#include "LookAndFeel.h"

namespace Element {
    
void LookAndFeel::drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area,
                                             bool isMouseOver, bool isMouseDown,
                                             ConcertinaPanel& panel, Component& comp)
{
    g.setColour (Colour (0xff323232));
    Rectangle<int> r (area.withSizeKeepingCentre (area.getWidth(), area.getHeight() - 2));
    g.fillRect (r);
}

}
