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

    //==============================================================================
   #if 0
    AlertWindow* createAlertWindow (const String& title, const String& message,
                                    const String& button1,
                                    const String& button2,
                                    const String& button3,
                                    AlertWindow::AlertIconType iconType,
                                    int numButtons, Component* associatedComponent) override;
    void drawAlertBox (Graphics&, AlertWindow&, const Rectangle<int>& textArea, TextLayout&) override;

    int getAlertWindowButtonHeight() override;
    Font getAlertWindowTitleFont() override;
    Font getAlertWindowMessageFont() override;
    Font getAlertWindowFont() override;
   #endif

    // rotary slider
    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, 
                           Slider& slider) override;

    void drawProgressBar (Graphics& g, ProgressBar& progressBar,
                          int width, int height, double progress, 
                          const String& textToShow) override;
    
    // MARK: Concertina Panel
    void drawConcertinaPanelHeader (Graphics&, const Rectangle<int>& area,
                                    bool isMouseOver, bool isMouseDown,
                                    ConcertinaPanel&, Component&) override;
    
    // MARK: ComboBox
    Font getComboBoxFont (ComboBox& box) override;

    // MARK: Labels
    // virtual void drawLabel (Graphics&, Label&) = 0;
    Font getLabelFont (Label&) override;
    // virtual BorderSize<int> getLabelBorderSize (Label&) = 0;

    // MARK: Property Panel
    void drawPropertyPanelSectionHeader (Graphics&, const String& name, bool isOpen, int width, int height) override;
    void drawPropertyComponentBackground (Graphics&, int width, int height, PropertyComponent&) override;
    void drawPropertyComponentLabel (Graphics&, int width, int height, PropertyComponent&) override;
    Rectangle<int> getPropertyComponentContentPosition (PropertyComponent&) override;
    
    // MARK: Toggle buttons

    Path getTickShape (float height) override;
    Path getCrossShape (float height) override;
    void drawToggleButton (Graphics&, ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, 
                           bool shouldDrawButtonAsDown) override;
    void drawTickBox (Graphics&, Component&,
                      float x, float y, float w, float h,
                      bool ticked, bool isEnabled,
                      bool shouldDrawButtonAsHighlighted, 
                      bool shouldDrawButtonAsDown) override;

    void changeToggleButtonWidthToFitText (ToggleButton&) override;
    
    // slider
    Label* createSliderTextBox (Slider& slider) override
    {
        auto l = LookAndFeel_V2::createSliderTextBox (slider);
        l->setFont (Font (13.f));
        return l;
    }

    // MARK: Treeview
    void drawTreeviewPlusMinusBox (Graphics&, const Rectangle<float> &area, Colour backgroundColour, bool isOpen, bool isMouseOver) override;
};

}
