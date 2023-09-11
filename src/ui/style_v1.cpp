// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/ui/style.hpp>
#include <element/ui/simplemeter.hpp>

#include "ui/buttons.hpp"
#include "ui/midiblinker.hpp"
#include "BinaryData.h"

// clang-format off

using namespace juce;

namespace element {

const Colour Colors::elemental = Colour (0xff4765a0);
const Colour Colors::toggleBlue = Colour (0xff33aaf9);
const Colour Colors::toggleGreen = Colour (0xff92e75e);
const Colour Colors::toggleOrange = Colour (0xfffaa63a);
const Colour Colors::toggleRed = Colour (0xffff0000);

const Colour Colors::elementBlue = Colors::elemental;
const Colour Colors::backgroundColor = Colour ((uint32) LookAndFeel_E1::defaultBackgroundColor);
const Colour Colors::widgetBackgroundColor = Colour (0xff3b3b3b);
const Colour Colors::contentBackgroundColor = Colors::widgetBackgroundColor.darker().darker();
// const Colour Colors::contentBackgroundColor = Colour (0xff212125);

const Colour Colors::textColor = Colour ((uint32) LookAndFeel_E1::defaultTextColor);
const Colour Colors::textActiveColor = Colour ((uint32) LookAndFeel_E1::defaultTextActiveColor);
const Colour Colors::textBoldColor = Colour ((uint32) LookAndFeel_E1::defaultTextBoldColor);
const Colour Colors::highlightBackgroundColor = Colors::textColor.darker (0.6000006f).withAlpha (0.6f);

LookAndFeel_E1::LookAndFeel_E1()
{
    setColour (ResizableWindow::backgroundColourId, Colors::widgetBackgroundColor.darker (0.3f));

    // Text Buttons
    setColour (TextButton::buttonColourId, Colour (0xff525252));
    setColour (TextButton::buttonOnColourId, Colour (0xff525252));
    setColour (TextButton::textColourOffId, Colours::white);
    setColour (TextButton::textColourOnId, Colours::white);

    // PopupMenu Styling
    setColour (PopupMenu::backgroundColourId, Colour (0xfff0f0f0));
    setColour (PopupMenu::textColourId, Colour (0xff1d1d1e));
    setColour (PopupMenu::headerTextColourId, Colour (0xff1d1d1e));
    setColour (PopupMenu::highlightedBackgroundColourId, Colors::elemental);
    setColour (PopupMenu::highlightedTextColourId, Colour (0xfff0f0f0));

    // ComboBox Styling
    setColour (ComboBox::backgroundColourId, Colours::black);
    setColour (ComboBox::outlineColourId, Colours::black.brighter (0.2f));
    setColour (ComboBox::buttonColourId, Colours::black.brighter (0.2f));
    setColour (ComboBox::textColourId, Colour ((uint32) defaultTextActiveColor));
    setColour (ComboBox::arrowColourId, Colour ((uint32) defaultTextColor));

    // Meter Styling
    typedef SimpleMeter Meter;
    setColour (Meter::levelOverColourId, Colours::red);
    setColour (Meter::level0dBColourId, Colours::red);
    setColour (Meter::level3dBColourId, Colours::yellow);
    setColour (Meter::level6dBColourId, Colours::yellow);
    setColour (Meter::level10dBColourId, Colours::green);
    setColour (Meter::backgroundColourId, Colours::black);

    // ListBox Styling
    setColour (ListBox::backgroundColourId, Colour (0x00000000));
    setColour (ListBox::textColourId, Colors::textColor);

    // Text Editor
    setColour (TextEditor::backgroundColourId, Colors::backgroundColor);
    setColour (TextEditor::highlightColourId, Colors::backgroundColor.brighter());
    setColour (TextEditor::highlightColourId, Colors::highlightBackgroundColor);
    setColour (TextEditor::highlightedTextColourId, Colors::textColor.contrasting());
    setColour (TextEditor::textColourId, Colors::textColor);

    // Toolbar Styling
    setColour (Toolbar::backgroundColourId, Colors::backgroundColor.brighter (0.05f));
    setColour (Toolbar::buttonMouseDownBackgroundColourId, Colors::backgroundColor.brighter (0.1f));
    setColour (Toolbar::buttonMouseOverBackgroundColourId, Colors::backgroundColor.darker (0.046f));

    // Alert Window
    setColour (AlertWindow::backgroundColourId, Colors::backgroundColor);
    setColour (AlertWindow::textColourId, Colors::textColor);

    // Label
    setColour (Label::textColourId, Colors::textColor);

    // search path component
    setColour (FileSearchPathListComponent::backgroundColourId, Colors::backgroundColor);

    // Tree View
    setColour (TreeView::backgroundColourId, Colour (0x00000000));
    setColour (TreeView::linesColourId, Colors::textColor);
    setColour (TreeView::dragAndDropIndicatorColourId, Colours::orange.darker());
    setColour (TreeView::selectedItemBackgroundColourId, Colors::elemental.darker (0.6000006f));

    // Carrot
    setColour (CaretComponent::caretColourId, Colors::toggleOrange.brighter (0.20f));

    // Text Editor
    setColour (TextEditor::textColourId, Colors::textColor);
    setColour (TextEditor::highlightColourId, Colors::elemental.brighter (0.31f));
    setColour (TextEditor::highlightedTextColourId, Colours::black.brighter (0.22f));
    setColour (TextEditor::outlineColourId, Colours::black);
    setColour (TextEditor::focusedOutlineColourId, Colors::toggleBlue.darker (0.002f).withAlpha (0.6f));

    // Slider
    setColour (Slider::thumbColourId, Colours::black);
    setColour (Slider::textBoxTextColourId, Colors::textColor);
    setColour (Slider::trackColourId, Colours::black);
    setColour (Slider::textBoxBackgroundColourId, findColour (TextEditor::backgroundColourId));
    setColour (Slider::textBoxHighlightColourId, findColour (TextEditor::highlightColourId));
    setColour (Slider::textBoxOutlineColourId, findColour (TextEditor::outlineColourId));
    setColour (Slider::textBoxTextColourId, findColour (TextEditor::textColourId));

    // Digital meter styling
    setColour (SimpleMeter::levelOverColourId, Colours::yellow.darker());
    setColour (SimpleMeter::level0dBColourId, Colours::yellowgreen);
    setColour (SimpleMeter::level3dBColourId, Colours::lightgreen);
    setColour (SimpleMeter::level6dBColourId, Colours::green);
    setColour (SimpleMeter::level10dBColourId, Colours::darkgreen.darker());
    setColour (SimpleMeter::backgroundColourId, Colours::transparentBlack);
    setColour (SimpleMeter::foregroundColourId, Colours::transparentWhite);

    // ProgressBar
    setColour (ProgressBar::foregroundColourId, Colors::elemental);
    setColour (ProgressBar::backgroundColourId, findColour (DocumentWindow::backgroundColourId).darker());

#if 0 // JUCE_MODULE_AVAILABLE_kv_engines
    setColour (TimelineComponent::bodyBackgroundColourId, findColour (ResizableWindow::backgroundColourId));
    setColour (TimelineComponent::bodyBackgroundColourId, findColour (ResizableWindow::backgroundColourId));
#endif

#if 0
    setColour (ListBox::backgroundColourId, Colour (0xff222222));

    setColour (TreeView::selectedItemBackgroundColourId, Colour (0x301111ee));
    setColour (TreeView::backgroundColourId, Colour (0xff222222));

    const Colour textButtonColour (0xffeeeeff);

    setColour (TextButton::buttonColourId, textButtonColour);
    setColour (ComboBox::buttonColourId, textButtonColour);
    setColour (ScrollBar::thumbColourId, Colour::greyLevel (0.8f).contrasting().withAlpha (0.13f));
#endif

    // Element Colors
    setColour (Style::backgroundColorId, Colour (0xff16191a));
    setColour (Style::backgroundHighlightColorId, Colour (0xffcccccc).darker (0.6000006f).withAlpha (0.6f));

    setColour (Style::widgetBackgroundColorId, Colour (0xff3b3b3b));
    setColour (Style::contentBackgroundColorId, Colour (0xff3b3b3b).darker (0.6f));

    setColour (Style::textColorId, Colour (0xffcccccc));
    setColour (Style::textActiveColorId, Colour (0xffe5e5e5));
    setColour (Style::textBoldColorId, Colour (0xffe4e4e4));

    // setColour (ResizableWindow::backgroundColourId, Colors::widgetBackgroundColor.darker(.3));
    // setColour (CaretComponent::caretColourId, Colors::toggleOrange.brighter (0.20f));

    // Property Component
    setColour (PropertyComponent::labelTextColourId, Colors::textColor);
    setColour (PropertyComponent::backgroundColourId, Colors::backgroundColor.brighter (0.02f));

    // // Text Editor
    // setColour (TextEditor::textColourId,            Colors::textColor);
    // setColour (TextEditor::highlightColourId,       Colors::elemental.brighter (0.31f));
    // setColour (TextEditor::highlightedTextColourId, Colours::black.brighter(0.22f));
    // setColour (TextEditor::outlineColourId,         Colours::black);
    // setColour (TextEditor::focusedOutlineColourId,  Colors::toggleBlue.darker (0.002).withAlpha (0.6f));

    setColour (Label::textWhenEditingColourId, findColour (TextEditor::textColourId).darker (0.003f));

    setColour (TextPropertyComponent::outlineColourId, findColour (TextEditor::outlineColourId));
    setColour (TextPropertyComponent::backgroundColourId, findColour (TextEditor::backgroundColourId));
    setColour (TextPropertyComponent::textColourId, findColour (TextEditor::textColourId));

    setColour (ToggleButton::textColourId, Colors::textColor);

    // Boolean property comp
    setColour (BooleanPropertyComponent::backgroundColourId, findColour (TextEditor::backgroundColourId));
    setColour (BooleanPropertyComponent::outlineColourId, Colours::black);

    // Setting Button
    setColour (SettingButton::backgroundColourId, Colors::widgetBackgroundColor.brighter());
    setColour (SettingButton::backgroundOnColourId, Colors::toggleOrange);
    setColour (SettingButton::textColourId, Colours::black);
    setColour (SettingButton::textDisabledColourId, Colours::darkgrey);

    // MIDI Blinkers
    setColour (MidiBlinker::backgroundColourId, findColour (SettingButton::backgroundColourId));
    setColour (MidiBlinker::outlineColourId, Colors::widgetBackgroundColor.brighter().brighter());

    // Tree View
    setColour (TreeView::selectedItemBackgroundColourId, Colors::elemental.darker (0.6000006f));
    setColour (TreeView::backgroundColourId, Colors::backgroundColor);

    // Keymap Editor
    setColour (KeyMappingEditorComponent::textColourId, Colors::textColor);
    setColour (KeyMappingEditorComponent::backgroundColourId, findColour (TreeView::backgroundColourId));

    // Directory Contents Display
    setColour (DirectoryContentsDisplayComponent::textColourId, Colors::textColor);
    setColour (DirectoryContentsDisplayComponent::highlightColourId, Colors::elemental.darker (0.6000006f));

    // List Box
    setColour (ListBox::textColourId, Colors::textColor);

    // // Slider
    //                                                    and feel class how this is used. */
    //     trackColourId               = 0x1001310,  /**< The colour to draw the groove that the thumb moves along. */
    //     rotarySliderFillColourId    = 0x1001311,  /**< For rotary sliders, this colour fills the outer curve. */
    //     rotarySliderOutlineColourId = 0x1001312,  /**< For rotary sliders, this colour is used to draw the outer curve's outline. */

    //     textBoxOutlineColourId      = 0x1001700   /**< The colour to use for a border around the text-editor box. */

    setColour (Slider::backgroundColourId,          Colours::black.brighter (0.15f));
    setColour (Slider::thumbColourId,               Colours::black.brighter (0.12f));
    setColour (Slider::trackColourId,               Colours::black.brighter (0.05f));
    
    setColour (Slider::rotarySliderFillColourId,    Colors::toggleBlue.darker (0.3f));

    setColour (Slider::textBoxTextColourId,         Colors::textColor);
    setColour (Slider::textBoxBackgroundColourId,   findColour (TextEditor::backgroundColourId));
    setColour (Slider::textBoxHighlightColourId,    findColour (TextEditor::highlightColourId));
    setColour (Slider::textBoxOutlineColourId,      findColour (TextEditor::outlineColourId));

    // Hyperlink button
    setColour (HyperlinkButton::textColourId, Colors::toggleBlue);

    // DockItem
    // setColour (DockItem::selectedHighlightColourId, Colors::toggleBlue);

    // // ProgressBar
    // setColour (ProgressBar::foregroundColourId, Colors::elemental);
    // setColour (ProgressBar::backgroundColourId, findColour (
    //     DocumentWindow::backgroundColourId).darker());

    // ToggleButton
    setColour (ToggleButton::tickColourId, Colors::toggleBlue.darker());

    

    // Scrollbar
    setColour (ScrollBar::thumbColourId, Colour::greyLevel (0.25f));

    // code editor. TODO
    setColour (CodeEditorComponent::backgroundColourId, findColour (Style::widgetBackgroundColorId).darker (0.6f));
    setColour (CodeEditorComponent::highlightColourId, Colour (0xff1b5381));
    setColour (CodeEditorComponent::defaultTextColourId, Colour (0xffc4c4c4));
    setColour (CodeEditorComponent::lineNumberBackgroundId, findColour (Style::widgetBackgroundColorId).darker (0.55f));
    setColour (CodeEditorComponent::lineNumberTextId, Colour (0xff555555));


    setColour (0x1000440 /*lassoFillColourId*/, Colours::transparentWhite.withAlpha (0.24f));
    setColour (0x1000441 /*lassoOutlineColourId*/, Colours::whitesmoke.withAlpha (0.44f));
}

LookAndFeel_E1::~LookAndFeel_E1() {}

Typeface::Ptr LookAndFeel_E1::getTypefaceForFont (const Font& font)
{
#if JUCE_LINUX
    if (font.getTypefaceName() == Font::getDefaultSansSerifFontName())
    {
        return Typeface::createSystemTypefaceFor (
            BinaryData::RobotoRegular_ttf, BinaryData::RobotoRegular_ttfSize);
    }
    else if (font.getTypefaceName() == Font::getDefaultMonospacedFontName())
    {
        Font f (font);
        if (defaultMonospaceName.isEmpty())
        {
            const StringArray possible ("Courier 10 Pitch");
            const auto names = Font::findAllTypefaceNames();
            for (const auto& name : possible)
                if (names.contains (name))
                {
                    defaultMonospaceName = name;
                    break;
                }
            if (defaultMonospaceName.isEmpty())
                defaultMonospaceName = names[0];
        }

        f.setTypefaceName (defaultMonospaceName);
        f.setTypefaceStyle ("Regular");
        return Typeface::createSystemTypefaceFor (f);
    }
#endif
    return LookAndFeel_V2::getTypefaceForFont (font);
}

int LookAndFeel_E1::getDefaultScrollbarWidth() { return 12; }
bool LookAndFeel_E1::areScrollbarButtonsVisible() { return false; }

//=============================================================================
int LookAndFeel_E1::getSliderThumbRadius (Slider& slider)
{
    return jmin (12, slider.isHorizontal() 
        ? static_cast<int> ((float) slider.getHeight() * 0.77f) 
        : static_cast<int> ((float) slider.getWidth() * 0.77f));
}

void LookAndFeel_E1::drawRotarySlider (Graphics& g, int x, int y, int width, int height, 
                                       float sliderPos, const float rotaryStartAngle, 
                                       const float rotaryEndAngle, Slider& slider)
{
    Style::drawDial (g, x, y, width, height, sliderPos, 0.f, 
                     rotaryStartAngle, rotaryEndAngle, slider);
}

void LookAndFeel_E1::drawLinearSlider (Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle st, Slider& sl)
{
    juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, st, sl);
}

void LookAndFeel_E1::drawLinearSliderBackground (Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider)
{
    LookAndFeel_V4::drawLinearSliderBackground (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
#if 0
    const float sliderRadius = (float) (getSliderThumbRadius (slider) - 4);

    const Colour trackColour (slider.findColour (Slider::trackColourId));
    const Colour gradCol1 (trackColour.overlaidWith (Colour (slider.isEnabled() ? 0x13000000 : 0x09000000)));
    const Colour gradCol2 (trackColour.overlaidWith (Colour (0x06000000)));
    Path indent;
    const float cornerSize = 1.f;

    if (slider.isHorizontal())
    {
        auto iy = y + height * 0.5f - sliderRadius * 0.5f;
        g.setGradientFill (ColourGradient::vertical (gradCol1, iy, gradCol2, iy + sliderRadius));
        indent.addRoundedRectangle (x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius, cornerSize);
    }
    else
    {
        auto ix = x + width * 0.5f - sliderRadius * 0.5f;
        g.setGradientFill (ColourGradient::horizontal (gradCol1, ix, gradCol2, ix + sliderRadius));
        indent.addRoundedRectangle (ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius, cornerSize);
    }

    g.fillPath (indent);

    g.setColour (trackColour.contrasting (0.5f));
    g.strokePath (indent, PathStrokeType (0.5f));
#endif
}

//==============================================================================
Font LookAndFeel_E1::getComboBoxFont (ComboBox& box)
{
    return Font (jmin (12.0f, box.getHeight() * 0.85f));
}

// Label
Font LookAndFeel_E1::getLabelFont (Label& label)
{
    if (nullptr != dynamic_cast<PropertyComponent*> (label.getParentComponent()))
        label.setFont (Font (13.f));
    return label.getFont();
}

Font LookAndFeel_E1::getMenuBarFont (MenuBarComponent&, int, const String&)
{
    return Font (14.f);
}

//=====

Path LookAndFeel_E1::getTickShape (float height)
{
    static const unsigned char pathData[] = { 110, 109, 32, 210, 202, 64, 126, 183, 148, 64, 108, 39, 244, 247, 64, 245, 76, 124, 64, 108, 178, 131, 27, 65, 246, 76, 252, 64, 108, 175, 242, 4, 65, 246, 76, 252, 64, 108, 236, 5, 68, 65, 0, 0, 160, 180, 108, 240, 150, 90, 65, 21, 136, 52, 63, 108, 48, 59, 16, 65, 0, 0, 32, 65, 108, 32, 210, 202, 64, 126, 183, 148, 64, 99, 101, 0, 0 };

    Path path;
    path.loadPathFromData (pathData, sizeof (pathData));
    path.scaleToFit (0, 0, height * 2.0f, height, true);

    return path;
}

Path LookAndFeel_E1::getCrossShape (float height)
{
    static const unsigned char pathData[] = { 110, 109, 51, 51, 255, 66, 0, 0, 0, 0, 108, 205, 204, 13, 67, 51, 51, 99, 65, 108, 0, 0, 170, 66, 205, 204, 141, 66, 108, 51, 179, 13, 67, 52, 51, 255, 66, 108, 0, 0, 255, 66, 205, 204, 13, 67, 108, 205, 204, 141, 66, 0, 0, 170, 66, 108, 52, 51, 99, 65, 51, 179, 13, 67, 108, 0, 0, 0, 0, 51, 51, 255, 66, 108, 205, 204, 98, 66, 204, 204, 141, 66, 108, 0, 0, 0, 0, 51, 51, 99, 65, 108, 51, 51, 99, 65, 0, 0, 0, 0, 108, 205, 204, 141, 66, 205, 204, 98, 66, 108, 51, 51, 255, 66, 0, 0, 0, 0, 99, 101, 0, 0 };

    Path path;
    path.loadPathFromData (pathData, sizeof (pathData));
    path.scaleToFit (0, 0, height * 2.0f, height, true);

    return path;
}

void LookAndFeel_E1::drawToggleButton (Graphics& g, ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto fontSize = jmin (13.0f, button.getHeight() * 0.75f);
    auto tickWidth = fontSize * 1.1f;

    drawTickBox (g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth, button.getToggleState(), button.isEnabled(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    g.setColour (button.findColour (ToggleButton::textColourId));
    g.setFont (fontSize);

    if (! button.isEnabled())
        g.setOpacity (0.5f);

    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().withTrimmedLeft (roundToInt (tickWidth) + 10).withTrimmedRight (2),
                      Justification::centredLeft,
                      10);
}

void LookAndFeel_E1::drawTickBox (Graphics& g, Component& component, float x, float y, float w, float h, const bool ticked, const bool isEnabled, const bool shouldDrawButtonAsHighlighted, const bool shouldDrawButtonAsDown)
{
    ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    Rectangle<float> tickBounds (x, y, w, h);

    g.setColour (component.findColour (ToggleButton::tickDisabledColourId));
    g.drawRoundedRectangle (tickBounds, 4.0f, 1.0f);

    if (ticked)
    {
        g.setColour (component.findColour (ToggleButton::tickColourId));
        auto tick = getTickShape (0.75f);
        g.fillPath (tick, tick.getTransformToScaleToFit (tickBounds.reduced (4, 5).toFloat(), false));
    }
}

void LookAndFeel_E1::changeToggleButtonWidthToFitText (ToggleButton& button)
{
    auto fontSize = jmin (15.0f, button.getHeight() * 0.75f);
    auto tickWidth = fontSize * 1.1f;

    Font font (fontSize);

    button.setSize (font.getStringWidth (button.getButtonText()) + roundToInt (tickWidth) + 14, button.getHeight());
}

// MARK: Property Panel
inline static auto getPanelSpacing() { return 1.4f; }

void LookAndFeel_E1::drawPropertyPanelSectionHeader (Graphics& g, const String& name, bool isOpen, int width, int height)
{
    Rectangle<float> lb (0.f, 0.f, (float)width, (float)height);
    g.setColour (Colors::widgetBackgroundColor.brighter());
    g.fillRect (lb.withHeight (lb.getHeight() - getPanelSpacing()));

    auto buttonSize = height * 0.75f;
    auto buttonIndent = (height - buttonSize) * 0.5f;

    drawTreeviewPlusMinusBox (g, Rectangle<float> (buttonIndent, buttonIndent, buttonSize, buttonSize), 
                                Colours::white, isOpen, false);

    g.setColour (Colors::textColor);
    g.drawText (name, 0, 0, width, height, 
                Justification::centred);
}

void LookAndFeel_E1::drawPropertyComponentBackground (Graphics& g, int width, int height, PropertyComponent& pc)
{
    const auto r = getPropertyComponentContentPosition (pc);
    g.setColour (Colors::widgetBackgroundColor.darker (0.0015f));
    g.fillRect (0, 0, r.getX(), height - 1);

    g.setColour (pc.findColour (PropertyComponent::backgroundColourId));
    g.fillRect (r.getX(), 0, width - r.getX(), height - 1);
}

void LookAndFeel_E1::drawPropertyComponentLabel (Graphics& g, int width, int height, PropertyComponent& component)
{
    g.setColour (component.findColour (PropertyComponent::labelTextColourId)
                    .withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.6f));

    // g.setFont (jmin (height, 24) * 0.55f);
    g.setFont (12.f);

    auto r = getPropertyComponentContentPosition (component);

    g.drawFittedText (component.getName(),
                        3, 
                        r.getY(), 
                        r.getX() - 5, 
                        r.getHeight(),
                        Justification::centredLeft, 
                        2);
}

Rectangle<int> LookAndFeel_E1::getPropertyComponentContentPosition (PropertyComponent& component)
{
    const auto textW = jmin (130, component.getWidth() / 2);
    return { textW, 0, component.getWidth() - textW, component.getHeight() - 1 };
}

//========
void LookAndFeel_E1::drawStretchableLayoutResizerBar (Graphics& g, int /*w*/, int /*h*/, bool /*isVerticalBar*/, bool isMouseOver, bool isMouseDragging)
{
    if (isMouseOver || isMouseDragging)
        g.fillAll (Colors::elemental.withAlpha (0.4f));
}

void LookAndFeel_E1::drawScrollbar (Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    Path thumbPath;

    if (thumbSize > 0)
    {
        const float thumbIndent = (isScrollbarVertical ? width : height) * 0.25f;
        const float thumbIndentx2 = thumbIndent * 2.0f;

        if (isScrollbarVertical)
            thumbPath.addRoundedRectangle (x + thumbIndent, thumbStartPosition + thumbIndent, width - thumbIndentx2, thumbSize - thumbIndentx2, (width - thumbIndentx2) * 0.5f);
        else
            thumbPath.addRoundedRectangle (thumbStartPosition + thumbIndent, y + thumbIndent, thumbSize - thumbIndentx2, height - thumbIndentx2, (height - thumbIndentx2) * 0.5f);
    }

    Colour thumbCol (scrollbar.findColour (ScrollBar::thumbColourId, true));

    if (isMouseOver || isMouseDown)
        thumbCol = thumbCol.withMultipliedAlpha (2.0f);

    g.setColour (thumbCol);
    g.fillPath (thumbPath);

    g.setColour (thumbCol.contrasting ((isMouseOver || isMouseDown) ? 0.2f : 0.1f));
    g.strokePath (thumbPath, PathStrokeType (1.0f));
}

void LookAndFeel_E1::drawConcertinaPanelHeader (Graphics& g, const Rectangle<int>& area, bool isMouseOver, bool isMouseDown, ConcertinaPanel& concertina, Component& panel)
{
    g.setColour (Colour (0xff292929));
    Rectangle<int> r (area.withSizeKeepingCentre (area.getWidth(), area.getHeight() - 2));
    g.fillRect (r);

    const Colour bkg (Colors::widgetBackgroundColor.brighter (0.1f));

    // g.setGradientFill (ColourGradient (Colours::white.withAlpha (isMouseOver ? 0.4f : 0.2f), 0, (float) area.getY(), Colours::darkgrey.withAlpha (0.2f), 0, (float) area.getBottom(), false));
    g.fillAll();

    g.setColour (bkg.contrasting().withAlpha (0.04f));
    g.fillRect (area.withHeight (1));
    g.fillRect (area.withTop (area.getBottom() - 1));

    g.setColour (findColour (Label::textColourId));
    g.setFont (Font (area.getHeight() * 0.62f).boldened());
    g.drawFittedText (panel.getName().toUpperCase(), 8, 0, area.getWidth() - 6, area.getHeight(), Justification::centredLeft, 1);
}

// ProgressBar

void LookAndFeel_E1::drawLinearProgressBar (Graphics& g, ProgressBar& progressBar, int width, int height, double progress, const String& textToShow)
{
    auto background = progressBar.findColour (ProgressBar::backgroundColourId);
    auto foreground = progressBar.findColour (ProgressBar::foregroundColourId);

    auto barBounds = progressBar.getLocalBounds().toFloat();

    g.setColour (background);
    g.fillRoundedRectangle (barBounds, progressBar.getHeight() * 0.5f);

    if (progress >= 0.0f && progress <= 1.0f)
    {
        Path p;
        p.addRoundedRectangle (barBounds, progressBar.getHeight() * 0.5f);
        g.reduceClipRegion (p);

        barBounds.setWidth (barBounds.getWidth() * (float) progress);
        g.setColour (foreground);
        g.fillRoundedRectangle (barBounds, progressBar.getHeight() * 0.5f);
    }
    else
    {
        // spinning bar..
        g.setColour (background);

        auto stripeWidth = height * 2;
        auto position = static_cast<int> (Time::getMillisecondCounter() / 15) % stripeWidth;

        Path p;

        for (auto x = static_cast<float> (-position); x < width + stripeWidth; x += stripeWidth)
            p.addQuadrilateral (x, 0.0f, x + stripeWidth * 0.5f, 0.0f, x, static_cast<float> (height), x - stripeWidth * 0.5f, static_cast<float> (height));

        Image im (Image::ARGB, width, height, true);

        {
            Graphics g2 (im);
            g2.setColour (foreground);
            g2.fillRoundedRectangle (barBounds, progressBar.getHeight() * 0.5f);
        }

        g.setTiledImageFill (im, 0, 0, 0.85f);
        g.fillPath (p);
    }

    if (textToShow.isNotEmpty())
    {
        //        g.setColour (Colour::contrasting (background, foreground));
        g.setColour (Colours::white);
        g.setFont (height * 0.6f);

        g.drawText (textToShow, 0, 0, width, height, Justification::centred, false);
    }
}

//==============================================================================
void LookAndFeel_E1::drawProgressBar (Graphics& g, ProgressBar& progressBar, int width, int height, double progress, const String& textToShow)
{
    drawLinearProgressBar (g, progressBar, width, height, progress, textToShow);
}

void Style::drawVerticalText (juce::Graphics& g,
                              const juce::String& text,
                              const juce::Rectangle<int> area,
                              juce::Justification justification)
{
    using namespace juce;

    auto r = area;
    Graphics::ScopedSaveState savestate (g);

    if (justification == Justification::centred) {
        g.setOrigin (r.getX(), r.getY());
        g.addTransform (AffineTransform().rotated (
            MathConstants<float>::pi / 2.0f, 0.0f, 0.0f));
        g.drawText (text,
                    0,
                    -r.getWidth(),
                    r.getHeight(),
                    r.getWidth(),
                    justification,
                    false);
    } else if (justification == Justification::left || justification == Justification::centredLeft || justification == Justification::topLeft || justification == Justification::bottomLeft) {
        g.setOrigin (r.getX(), r.getY());
        g.addTransform (AffineTransform().rotated (
            MathConstants<float>::pi / 2.0f, 0.0f, 0.0f));
        g.drawText (text,
                    0,
                    -r.getWidth(),
                    r.getHeight(),
                    r.getWidth(),
                    justification,
                    false);
    } else if (justification == Justification::right || justification == Justification::centredRight || justification == Justification::topRight || justification == Justification::bottomRight) {
        g.setOrigin (r.getX(), r.getY());
        g.addTransform (AffineTransform().rotated (
            -MathConstants<float>::pi / 2.0f, 0.0f, (float) r.getHeight()));
        g.drawText (text,
                    0,
                    r.getHeight(),
                    r.getHeight(),
                    r.getWidth(),
                    justification,
                    false);
    } else {
        jassertfalse; // mode not supported
    }
}

void Style::drawButtonShape (Graphics& g, const Path& outline, Colour baseColour, float height)
{
    const float mainBrightness = baseColour.getBrightness();
    const float mainAlpha = baseColour.getFloatAlpha();

    g.setGradientFill (ColourGradient (baseColour.brighter (0.2f), 0.0f, 0.0f, baseColour.darker (0.25f), 0.0f, height, false));
    g.fillPath (outline);

    g.setColour (Colours::white.withAlpha (0.4f * mainAlpha * mainBrightness * mainBrightness));
    g.strokePath (outline, PathStrokeType (1.0f), AffineTransform::translation (0.0f, 1.0f).scaled (1.0f, (height - 1.6f) / height));

    g.setColour (Colours::black.withAlpha (0.4f * mainAlpha));
    g.strokePath (outline, PathStrokeType (1.0f));
}

void Style::drawDial (juce::Graphics& g, int x, int y, int width, int height, 
                      float sliderPos, const float anchorPos, 
                      const float rotaryStartAngle, const float rotaryEndAngle,
                      juce::Slider& slider)
{
    const float radius = jmin (width / 2, height / 2) - 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float anchor = rotaryStartAngle + anchorPos * (rotaryEndAngle - rotaryStartAngle);
    const float a1 = angle < anchor ? angle : anchor;
    const float a2 = angle < anchor ? anchor : angle;

    const bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

    if (radius > 12.0f)
    {
        if (slider.isEnabled())
            g.setColour (slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 1.0f : 0.7f));
        else
            g.setColour (Colour (0x80808080));

        const float thickness = 0.7f;

        {
            Path filledArc;
            filledArc.addPieSegment (rx, ry, rw, rw, a1, a2, thickness);
            g.fillPath (filledArc);
        }

        {
            const float innerRadius = radius * 0.2f;
            Path p;
            p.addTriangle (-innerRadius, 0.0f, 0.0f, -radius * thickness * 1.1f, innerRadius, 0.0f);

            p.addEllipse (-innerRadius, -innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

            g.fillPath (p, AffineTransform::rotation (angle).translated (centreX, centreY));
        }

        if (slider.isEnabled())
            g.setColour (slider.findColour (Slider::rotarySliderOutlineColourId));
        else
            g.setColour (Colour (0x80808080));

        Path outlineArc;
        outlineArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, thickness);
        outlineArc.closeSubPath();

        g.strokePath (outlineArc, PathStrokeType (slider.isEnabled() ? (isMouseOver ? 2.0f : 1.2f) : 0.3f));
    }
    else
    {
        if (slider.isEnabled())
            g.setColour (slider.findColour (Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 1.0f : 0.7f));
        else
            g.setColour (Colour (0x80808080));

        Path p;
        p.addEllipse (-0.4f * rw, -0.4f * rw, rw * 0.8f, rw * 0.8f);
        PathStrokeType (rw * 0.1f).createStrokedPath (p, p);

        p.addLineSegment (Line<float> (0.0f, 0.0f, 0.0f, -radius), rw * 0.2f);

        g.fillPath (p, AffineTransform::rotation (angle).translated (centreX, centreY));
    }
}

static int getFaderThumbSize (juce::Slider& slider, bool length)
{
    auto size = std::min (13, slider.isHorizontal()
        ? static_cast<int> ((float) slider.getHeight() * 0.90f) 
        : static_cast<int> ((float) slider.getWidth() * 0.90f));

    if (length)
        size = std::min (24, size * 2);

    return size;
}

void Style::drawFader (Graphics& g, int x, int y, int width, int height,
                       float sliderPos,
                       float minSliderPos,
                       float maxSliderPos,
                       const Slider::SliderStyle style,
                       Slider& slider)
{
    auto trackWidth = jmin (4.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);

    Point<float> startPoint (slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));

    Point<float> endPoint (slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                            slider.isHorizontal() ? startPoint.y : (float) y);

    Path backgroundTrack;
    backgroundTrack.startNewSubPath (startPoint);
    backgroundTrack.lineTo (endPoint);
    g.setColour (slider.findColour (Slider::backgroundColourId));
    g.strokePath (backgroundTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

    Path valueTrack;
    Point<float> minPoint, maxPoint, thumbPoint;
    element::ignore (thumbPoint);

    auto kx = slider.isHorizontal() ? sliderPos : ((float) x + (float) width * 0.5f);
    auto ky = slider.isHorizontal() ? ((float) y + (float) height * 0.5f) : sliderPos;

    minPoint = startPoint;
    maxPoint = { kx, ky };

    auto thumbWidth = getFaderThumbSize (slider, false);
    auto thumbLength = getFaderThumbSize (slider, true);

    valueTrack.startNewSubPath (minPoint);
    valueTrack.lineTo (maxPoint);
    g.setColour (slider.findColour (Slider::trackColourId));
    g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });

    // draw the thumb
    auto thumbColor= slider.findColour (Slider::thumbColourId);
    g.setColour (thumbColor);
    auto tr = Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbLength));
    g.fillRoundedRectangle (tr.withCentre (maxPoint), 3.f);
    g.setColour (thumbColor.brighter(0.3f));
    g.drawRoundedRectangle (tr.withCentre (maxPoint), 3.f, 1.0f);
    g.setColour (juce::Colours::whitesmoke.darker(0.2f));
    tr = tr.withCentre (maxPoint);
    g.drawLine (tr.getX() + 1.f, tr.getCentreY(), tr.getRight() - 1.f, tr.getCentreY(), 1.2);
    // g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (maxPoint));
}

void LookAndFeel_E1::drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
    Colour baseColour (backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                           .withMultipliedAlpha (button.isEnabled() ? 0.9f : 0.5f));

    if (isButtonDown || isMouseOverButton)
        baseColour = baseColour.contrasting (isButtonDown ? 0.2f : 0.1f);

    const bool flatOnLeft = button.isConnectedOnLeft();
    const bool flatOnRight = button.isConnectedOnRight();
    const bool flatOnTop = button.isConnectedOnTop();
    const bool flatOnBottom = button.isConnectedOnBottom();

    const float width = button.getWidth() - 1.0f;
    const float height = button.getHeight() - 1.0f;
    const float cornerSize = 2.0f;

    Path outline;
    outline.addRoundedRectangle (0.5f, 0.5f, width, height, cornerSize, cornerSize, ! (flatOnLeft || flatOnTop), ! (flatOnRight || flatOnTop), ! (flatOnLeft || flatOnBottom), ! (flatOnRight || flatOnBottom));

    Style::drawButtonShape (g, outline, baseColour, height);
}

void LookAndFeel_E1::drawTableHeaderBackground (Graphics& g, TableHeaderComponent& header)
{
    Rectangle<int> r (header.getLocalBounds());

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRect (r.removeFromBottom (1));

    g.setColour (Colours::white.withAlpha (0.6f));
    g.fillRect (r);

    g.setColour (Colours::black.withAlpha (0.5f));

    for (int i = header.getNumColumns (true); --i >= 0;)
        g.fillRect (header.getColumnPosition (i).removeFromRight (1));
}

int LookAndFeel_E1::getTabButtonOverlap (int /*tabDepth*/) { return -1; }
int LookAndFeel_E1::getTabButtonSpaceAroundImage() { return 1; }

void LookAndFeel_E1::createTabTextLayout (const TabBarButton& button, float length, float depth, Colour colour, TextLayout& textLayout)
{
    Font font (depth * 0.5f);
    font.setUnderline (button.hasKeyboardFocus (false));

    AttributedString s;
    s.setJustification (Justification::centred);
    s.append (button.getButtonText().trim(), font, colour);

    textLayout.createLayout (s, length);
}

void LookAndFeel_E1::drawTabButton (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
    const Rectangle<int> activeArea (button.getActiveArea());

    const TabbedButtonBar::Orientation o = button.getTabbedButtonBar().getOrientation();

    const Colour bkg (button.getTabBackgroundColour());

    if (button.getToggleState())
    {
        g.setColour (bkg);
        g.fillRect (activeArea);
    }
    else
    {
        Point<int> p1, p2;

        switch (o)
        {
            case TabbedButtonBar::TabsAtBottom:
                p1 = activeArea.getBottomLeft();
                p2 = activeArea.getTopLeft();
                break;
            case TabbedButtonBar::TabsAtTop:
                p1 = activeArea.getTopLeft();
                p2 = activeArea.getBottomLeft();
                break;
            case TabbedButtonBar::TabsAtRight:
                p1 = activeArea.getTopRight();
                p2 = activeArea.getTopLeft();
                break;
            case TabbedButtonBar::TabsAtLeft:
                p1 = activeArea.getTopLeft();
                p2 = activeArea.getTopRight();
                break;
            default:
                jassertfalse;
                break;
        }

        g.setGradientFill (ColourGradient (bkg.brighter (0.2f), (float) p1.x, (float) p1.y, bkg.darker (0.1f), (float) p2.x, (float) p2.y, false));
        g.fillRect (activeArea);
    }

    g.setColour (bkg.contrasting (0.3f));
    Rectangle<int> r (activeArea);

    if (o != TabbedButtonBar::TabsAtBottom)
        g.fillRect (r.removeFromTop (1));
    if (o != TabbedButtonBar::TabsAtTop)
        g.fillRect (r.removeFromBottom (1));
    if (o != TabbedButtonBar::TabsAtRight)
        g.fillRect (r.removeFromLeft (1));
    if (o != TabbedButtonBar::TabsAtLeft)
        g.fillRect (r.removeFromRight (1));

    const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    const Colour col (bkg.contrasting().withMultipliedAlpha (alpha));

    const Rectangle<float> area (button.getTextArea().toFloat());

    float length = area.getWidth();
    float depth = area.getHeight();

    if (button.getTabbedButtonBar().isVertical())
        std::swap (length, depth);

    TextLayout textLayout;
    createTabTextLayout (button, length, depth, col, textLayout);

    AffineTransform t;

    switch (o)
    {
        case TabbedButtonBar::TabsAtLeft:
            t = t.rotated (juce::MathConstants<float>::pi * -0.5f).translated (area.getX(), area.getBottom());
            break;
        case TabbedButtonBar::TabsAtRight:
            t = t.rotated (juce::MathConstants<float>::pi * 0.5f).translated (area.getRight(), area.getY());
            break;
        case TabbedButtonBar::TabsAtTop:
        case TabbedButtonBar::TabsAtBottom:
            t = t.translated (area.getX(), area.getY());
            break;
        default:
            jassertfalse;
            break;
    }

    g.addTransform (t);
    textLayout.draw (g, Rectangle<float> (length, depth));
}

void LookAndFeel_E1::drawTreeviewPlusMinusBox (Graphics& g, const Rectangle<float>& area, Colour backgroundColour, bool isOpen, bool isMouseOver)
{
    Path p;
    if (isOpen)
    {
        p.addTriangle (0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f);
    }
    else
    {
        p.addTriangle (0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5);
    }

    g.setColour (backgroundColour.contrasting().withAlpha (isMouseOver ? 0.5f : 0.3f));
    g.fillPath (p, p.getTransformToScaleToFit (area.reduced (2, area.getHeight() / 4), true));
}

bool LookAndFeel_E1::areLinesDrawnForTreeView (TreeView&)
{
    return false;
}

int LookAndFeel_E1::getTreeViewIndentSize (TreeView&)
{
    return 20;
}

void LookAndFeel_E1::drawComboBox (Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box)
{
    g.fillAll (box.findColour (ComboBox::backgroundColourId));

    const Colour buttonColour (box.findColour (ComboBox::buttonColourId));

    if (box.isEnabled() && box.hasKeyboardFocus (false))
    {
        g.setColour (buttonColour);
        g.drawRect (0, 0, width, height, 2);
    }
    else
    {
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRect (0, 0, width, height);
    }

    const float outlineThickness = box.isEnabled() ? (isButtonDown ? 1.2f : 0.5f) : 0.3f;

    Path buttonShape;
    buttonShape.addRectangle (buttonX + outlineThickness,
                              buttonY + outlineThickness,
                              buttonW - outlineThickness * 2.0f,
                              buttonH - outlineThickness * 2.0f);

    Style::drawButtonShape (g,
                     buttonShape,
                     buttonColour.withMultipliedSaturation (box.hasKeyboardFocus (true) ? 1.3f : 0.9f).withMultipliedAlpha (box.isEnabled() ? 0.9f : 0.5f),
                     (float) height);

    if (box.isEnabled())
    {
        const float arrowX = 0.25f;
        const float arrowH = 0.2f;

        Path p;
        p.addTriangle (buttonX + buttonW * 0.5f, buttonY + buttonH * (0.45f - arrowH), buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.45f, buttonX + buttonW * arrowX, buttonY + buttonH * 0.45f);

        p.addTriangle (buttonX + buttonW * 0.5f, buttonY + buttonH * (0.55f + arrowH), buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.55f, buttonX + buttonW * arrowX, buttonY + buttonH * 0.55f);

        g.setColour (box.findColour (ComboBox::arrowColourId));
        g.fillPath (p);
    }
}

void LookAndFeel_E1::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (1, 1, box.getWidth() - box.getHeight(), box.getHeight() - 2);

    label.setFont (getComboBoxFont (box));
}

//==============================================================================
// MARK: Popup Menu

Font LookAndFeel_E1::getPopupMenuFont() { return LookAndFeel_V2::getPopupMenuFont(); }

void LookAndFeel_E1::drawPopupMenuBackground (Graphics& g, int width, int height)
{
    const Rectangle<float> r (0.f, 0.f, (float) width, (float) height);
    g.setColour (findColour (PopupMenu::backgroundColourId));
    g.fillRoundedRectangle (r, 0.0f);
}

void LookAndFeel_E1::getIdealPopupMenuItemSize (const String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
    LookAndFeel_V3::getIdealPopupMenuItemSize (text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight);
    if (isSeparator)
    {
        return;
    }

    idealHeight = 20;
}

// MARK: MenuBar

void LookAndFeel_E1::drawMenuBarBackground (Graphics& g, int width, int height, bool isMouseOverBar, MenuBarComponent& mbc)
{
    LookAndFeel_V3::drawMenuBarBackground (g, width, height, isMouseOverBar, mbc);
}

void LookAndFeel_E1::drawMenuBarItem (Graphics& g, int width, int height, int itemIndex, const String& itemText, bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar, MenuBarComponent& bar)
{
    LookAndFeel_V3::drawMenuBarItem (g, width, height, itemIndex, itemText, isMouseOverItem, isMenuOpen, isMouseOverBar, bar);
}

void LookAndFeel_E1::drawKeymapChangeButton (Graphics& g, int width, int height, Button& button, const String& keyDescription)
{
    const Colour textColour (button.findColour (0x100ad01 /*KeyMappingEditorComponent::textColourId*/, true));

    if (keyDescription.isNotEmpty())
    {
        if (button.isEnabled())
        {
            g.setColour (textColour.withAlpha (button.isDown() ? 0.4f : (button.isOver() ? 0.2f : 0.1f)));
            g.fillRoundedRectangle (button.getLocalBounds().toFloat(), 4.0f);
            g.drawRoundedRectangle (button.getLocalBounds().toFloat(), 4.0f, 1.0f);
        }

        g.setColour (textColour);
        g.setFont (height * 0.6f);
        g.drawFittedText (keyDescription, 4, 0, width - 8, height, Justification::centred, 1);
    }
    else
    {
        const float thickness = 7.0f;
        const float indent = 22.0f;

        Path p;
        p.addEllipse (0.0f, 0.0f, 100.0f, 100.0f);
        p.addRectangle (indent, 50.0f - thickness, 100.0f - indent * 2.0f, thickness * 2.0f);
        p.addRectangle (50.0f - thickness, indent, thickness * 2.0f, 50.0f - indent - thickness);
        p.addRectangle (50.0f - thickness, 50.0f + thickness, thickness * 2.0f, 50.0f - indent - thickness);
        p.setUsingNonZeroWinding (false);

        g.setColour (textColour.darker (0.1f).withAlpha (button.isDown() ? 0.7f : (button.isOver() ? 0.5f : 0.3f)));
        g.fillPath (p, p.getTransformToScaleToFit (2.0f, 2.0f, width - 4.0f, height - 4.0f, true));
    }

    if (button.hasKeyboardFocus (false))
    {
        g.setColour (textColour.withAlpha (0.4f));
        g.drawRect (0, 0, width, height);
    }
}

    // AlertWindow
juce::AlertWindow* LookAndFeel_E1::createAlertWindow (const juce::String& title, 
                                                      const juce::String& message,
                                                      const juce::String& button1,
                                                      const juce::String& button2,
                                                      const juce::String& button3,
                                                      juce::MessageBoxIconType iconType,
                                                      int numButtons,
                                                      juce::Component* associatedComponent)
{
    return LookAndFeel_V3::createAlertWindow (title, message, 
                                              button1, button2, button3, 
                                              iconType, numButtons, 
                                              associatedComponent);
}

void LookAndFeel_E1::drawAlertBox (juce::Graphics& g, 
                                   juce::AlertWindow& w, 
                                   const juce::Rectangle<int>& r, 
                                   juce::TextLayout& l)
{
    LookAndFeel_V3::drawAlertBox (g, w, r, l);
}

int LookAndFeel_E1::getAlertBoxWindowFlags()
{
    return LookAndFeel_V3::getAlertBoxWindowFlags();
}

juce::Array<int> LookAndFeel_E1::getWidthsForTextButtons (juce::AlertWindow& aw, const juce::Array<juce::TextButton*>& ab)
{
    return LookAndFeel_V3::getWidthsForTextButtons (aw, ab);
}

int LookAndFeel_E1::getAlertWindowButtonHeight()
{
    return LookAndFeel_V3::getAlertWindowButtonHeight();
}

juce::Font LookAndFeel_E1::getAlertWindowTitleFont()
{
    return LookAndFeel_V3::getAlertWindowTitleFont();
}

juce::Font LookAndFeel_E1::getAlertWindowMessageFont()
{
    return LookAndFeel_V3::getAlertWindowMessageFont();
}

juce::Font LookAndFeel_E1::getAlertWindowFont()
{
    return LookAndFeel_V3::getAlertWindowFont();
}

} // namespace element

// clang-format on
