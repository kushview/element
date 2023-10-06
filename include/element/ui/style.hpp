// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/gui_basics.hpp>

namespace element {

struct Colors {
    static const juce::Colour elemental;
    static const juce::Colour toggleBlue;
    static const juce::Colour toggleGreen;
    static const juce::Colour toggleOrange;
    static const juce::Colour toggleRed;

    static const juce::Colour elementBlue;

    static const juce::Colour backgroundColor;
    static const juce::Colour contentBackgroundColor;
    static const juce::Colour widgetBackgroundColor;

    static const juce::Colour textColor;
    static const juce::Colour textActiveColor;
    static const juce::Colour textBoldColor;

    static const juce::Colour highlightBackgroundColor;
};

struct Style {
    enum ColorID {
        backgroundColorId = 0x30000000,
        backgroundHighlightColorId,

        contentBackgroundColorId,
        widgetBackgroundColorId,

        textColorId,
        textActiveColorId,
        textBoldColorId,
    };

    /** The default font size that should be used in normal situations...
        e.g. standard Labels, Textboxes, ComboBoxes, etc etc */
    static constexpr float fontSizeDefault = 12.0f;

    /** Draws text rotated by 90 or -90 degrees */
    static void drawVerticalText (juce::Graphics& g,
                                  const juce::String& text,
                                  const juce::Rectangle<int> area,
                                  juce::Justification justification = juce::Justification::centredLeft);

    /** Draws the basic shape of a button. */
    static void drawButtonShape (juce::Graphics& g,
                                 const juce::Path& outline,
                                 juce::Colour baseColour,
                                 float height);

    /** Draws the basic shape of a dial or knob. */
    static void drawDial (juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float anchorPos,
                          const float rotaryStartAngle, const float rotaryEndAngle,
                          juce::Slider& slider);

    /** Draws a simple fader cap outline. */
    static void drawFader (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider);
};

// clang-format off
class JUCE_API LookAndFeel_E1 : public juce::LookAndFeel_V4 {
public:
    enum DefaultColorCodes {
        defaultBackgroundColor = 0xff16191A,

        defaultTextColor = 0xffcccccc,
        defaultTextActiveColor = 0xffe5e5e5,
        defaultTextBoldColor = 0xffe4e4e4,
        defaultTextEntryBackgroundColor = 0xff000000,
        defaultTextEntryForegroundColor = 0xffe5e5e5,
        defaultTabColor = 0xff1a1a1a,
        defaultTabOnColor = 0xff23252d,

        defaultMatrixCellOffColor = 0xff3b3b3b,
        defaultMatrixCellOnColor = 0xff9be94d,
        defaultMatrixCellHighlightColor = 0xff3b525b
    };

    LookAndFeel_E1();
    virtual ~LookAndFeel_E1();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    int getDefaultScrollbarWidth() override;

    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    juce::Font getLabelFont (juce::Label&) override;
    
    // Property Panel
    void drawPropertyPanelSectionHeader (juce::Graphics&, const juce::String& name, bool isOpen, int width, int height) override;
    void drawPropertyComponentBackground (juce::Graphics&, int width, int height, juce::PropertyComponent&) override;
    void drawPropertyComponentLabel (juce::Graphics&, int width, int height, juce::PropertyComponent&) override;
    juce::Rectangle<int> getPropertyComponentContentPosition (juce::PropertyComponent&) override;
    
    juce::Path getTickShape (float height) override;
    juce::Path getCrossShape (float height) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawTickBox (juce::Graphics&, juce::Component&, float x, float y, float w, float h, bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void changeToggleButtonWidthToFitText (juce::ToggleButton&) override;

    ////====///===//===///====
    // Buttons
    virtual void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override;

    virtual void drawTableHeaderBackground (juce::Graphics&, juce::TableHeaderComponent&) override;

    virtual bool areLinesDrawnForTreeView (juce::TreeView&) override;
    virtual void drawTreeviewPlusMinusBox (juce::Graphics&, const juce::Rectangle<float>& area, juce::Colour backgroundColour, bool isOpen, bool isMouseOver) override;
    virtual int getTreeViewIndentSize (juce::TreeView&) override;

    virtual void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    virtual void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    virtual void drawKeymapChangeButton (juce::Graphics& g, int width, int height, juce::Button& button, const juce::String& keyDescription) override;

    // Sliders
    virtual int getSliderThumbRadius (juce::Slider&) override;
    virtual void drawLinearSliderBackground (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float minSliderPos, float maxSliderPos, 
                                             const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    virtual void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                                   float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                                   juce::Slider& slider) override;
    virtual void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                                   float sliderPos, float minSliderPos, float maxSliderPos,
                                   const juce::Slider::SliderStyle, juce::Slider&) override;

    // Menus
    virtual juce::Font getPopupMenuFont() override;
    virtual void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;
    virtual void drawMenuBarBackground (juce::Graphics&, int width, int height, bool isMouseOverBar, juce::MenuBarComponent&) override;
    virtual void drawMenuBarItem (juce::Graphics&, int width, int height, int itemIndex, const juce::String& itemText, bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar, juce::MenuBarComponent&) override;
    virtual void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;

    virtual juce::Font getMenuBarFont (juce::MenuBarComponent&, int itemIndex, const juce::String& itemText) override;

    virtual int getTabButtonOverlap (int tabDepth) override;
    virtual int getTabButtonSpaceAroundImage() override;
    virtual void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

    virtual void drawStretchableLayoutResizerBar (juce::Graphics&, int w, int h, bool isVerticalBar, bool isMouseOver, bool isMouseDragging) override;

    bool areScrollbarButtonsVisible() override;
    virtual void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

    virtual void drawConcertinaPanelHeader (juce::Graphics&, const juce::Rectangle<int>& area, bool isMouseOver, bool isMouseDown, juce::ConcertinaPanel&, juce::Component&) override;

    static void createTabTextLayout (const juce::TabBarButton& button, float length, float depth, juce::Colour colour, juce::TextLayout&);

    // progress bar
    //==============================================================================
    virtual void drawLinearProgressBar (juce::Graphics& g, juce::ProgressBar& progressBar, int width, int height, double progress, const juce::String& textToShow);
    virtual void drawProgressBar (juce::Graphics& g, juce::ProgressBar& progressBar, int width, int height, double progress, const juce::String& textToShow) override;

    // slider
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto l = juce::LookAndFeel_V2::createSliderTextBox (slider);
        l->setFont (juce::Font (13.f));
        return l;
    }

    // AlertWindow
    virtual juce::AlertWindow* createAlertWindow (const juce::String& title, const juce::String& message,
                                            const juce::String& button1,
                                            const juce::String& button2,
                                            const juce::String& button3,
                                            juce::MessageBoxIconType iconType,
                                            int numButtons,
                                            juce::Component* associatedComponent) override;

    virtual void drawAlertBox (juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;

    virtual int getAlertBoxWindowFlags() override;

    virtual juce::Array<int> getWidthsForTextButtons (juce::AlertWindow&, const juce::Array<juce::TextButton*>&) override;
    virtual int getAlertWindowButtonHeight() override;

    virtual juce::Font getAlertWindowTitleFont() override;
    virtual juce::Font getAlertWindowMessageFont() override;
    virtual juce::Font getAlertWindowFont() override;

private:
    juce::Image backgroundTexture;
    juce::Colour backgroundTextureBaseColour;
    juce::String defaultSansSerifName,
        defaultMonospaceName;
};
// clang-format on

} // namespace element
