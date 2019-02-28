
#pragma once

#include "ElementApp.h"

namespace Element {

struct Colors
{
    static const Colour elemental;
    static const Colour toggleBlue;
    static const Colour toggleGreen;
    static const Colour toggleOrange;
    static const Colour toggleRed;
};

class LookAndFeel : public LookAndFeel_KV1
{
public:
    LookAndFeel();
    ~LookAndFeel() { }
    
    int getDefaultScrollbarWidth() override;

    void drawLinearProgressBar (Graphics& g, ProgressBar& progressBar,
                                             int width, int height,
                                             double progress, const String& textToShow);
    void drawProgressBar (Graphics& g, ProgressBar& progressBar,
                          int width, int height, double progress, const String& textToShow) override;
    // MARK: Concertina Panel
    void drawConcertinaPanelHeader (Graphics&, const Rectangle<int>& area,
                                    bool isMouseOver, bool isMouseDown,
                                    ConcertinaPanel&, Component&) override;
    
    // MARK: Property Panel
    void drawPropertyPanelSectionHeader (Graphics&, const String& name, bool isOpen, int width, int height) override;
    void drawPropertyComponentBackground (Graphics&, int width, int height, PropertyComponent&) override;
    void drawPropertyComponentLabel (Graphics&, int width, int height, PropertyComponent&) override;
    Rectangle<int> getPropertyComponentContentPosition (PropertyComponent&) override;
    
    // MARK: Toggle buttons

    Path getTickShape (float height) override;
    
    Path getCrossShape (float height) override;

    void drawToggleButton (Graphics&, ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawTickBox (Graphics&, Component&,
                      float x, float y, float w, float h,
                      bool ticked, bool isEnabled,
                      bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void changeToggleButtonWidthToFitText (ToggleButton&) override;

    // MARK: Treeview
    void drawTreeviewPlusMinusBox (Graphics&, const Rectangle<float> &area, Colour backgroundColour, bool isOpen, bool isMouseOver) override;

    // MARK: Sliders
    void drawLinearSliderBackground (Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     const Slider::SliderStyle style, Slider& slider) override;
};

}
