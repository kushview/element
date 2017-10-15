
#pragma once

#include "ElementApp.h"

namespace Element {

struct Colors
{
    static const Colour elemental;
    static const Colour toggleBlue;
    static const Colour toggleGreen;
    static const Colour toggleOrange;
};

class LookAndFeel : public LookAndFeel_KV1
{
public:
    LookAndFeel() { }
    ~LookAndFeel() { }
    
    // MARK: Concertina Panel
    void drawConcertinaPanelHeader (Graphics&, const Rectangle<int>& area,
                                    bool isMouseOver, bool isMouseDown,
                                    ConcertinaPanel&, Component&) override;
    
    // MARK: Property Panel
    void drawPropertyPanelSectionHeader (Graphics&, const String& name, bool isOpen, int width, int height) override;
    void drawPropertyComponentBackground (Graphics&, int width, int height, PropertyComponent&) override;
    void drawPropertyComponentLabel (Graphics&, int width, int height, PropertyComponent&) override;
    Rectangle<int> getPropertyComponentContentPosition (PropertyComponent&) override;
};

}
