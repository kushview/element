
#include "gui/ViewHelpers.h"

namespace Element {
namespace ViewHelpers {

typedef LookAndFeel_E1 LF;

void drawBasicTextRow (const String& text, Graphics& g, int w, int h, bool selected, int padding)
{
    g.saveState();
    
    if (selected)
    {
        g.setColour (LF::textColor.darker (0.6000006));
        g.setOpacity (0.60);
        g.fillRect (0, 0, w, h);
    }

    g.setColour ((selected) ? LF::textColor.contrasting() : LF::textColor);
    g.drawText (text, padding, 0, w - padding, h, Justification::centredLeft);

    g.restoreState();
}

}
}
