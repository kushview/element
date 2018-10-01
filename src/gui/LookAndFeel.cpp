
#include "LookAndFeel.h"
#include "gui/Buttons.h"

namespace Element {

const Colour Colors::elemental      = Colour (0xff4765a0);  // LookAndFeel_KV1::elementBlue;
const Colour Colors::toggleBlue     = Colour (0xff33aaf9);
const Colour Colors::toggleGreen    = Colour (0xff92e75e);
const Colour Colors::toggleOrange   = Colour (0xfffaa63a);
const Colour Colors::toggleRed      = Colour (0xffff0000);

LookAndFeel::LookAndFeel()
{
    
    setColour (ResizableWindow::backgroundColourId, widgetBackgroundColor.darker(.3));
    setColour (CaretComponent::caretColourId, Colors::toggleOrange.brighter (0.20f));

    // Property Component
    setColour (PropertyComponent::labelTextColourId, LookAndFeel::textColor);
    setColour (PropertyComponent::backgroundColourId, LookAndFeel::widgetBackgroundColor.brighter (0.002));
    
    // Slider
    setColour (Slider::thumbColourId,               Colours::black);
    setColour (Slider::textBoxTextColourId,         LookAndFeel::textColor);
    setColour (Slider::trackColourId,               Colours::red);
    
    // Text Editor
    setColour (TextEditor::textColourId,            textColor);
    setColour (TextEditor::highlightColourId,       Colors::elemental.brighter (0.31f));
    setColour (TextEditor::highlightedTextColourId, Colours::black.brighter(0.22f));
    setColour (TextEditor::outlineColourId,         Colours::black);
    setColour (TextEditor::focusedOutlineColourId,  Colors::toggleBlue.darker (0.002).withAlpha (0.6f));
    
    setColour (Label::textWhenEditingColourId,      findColour(TextEditor::textColourId).darker (0.003));

    setColour (TextPropertyComponent::outlineColourId,      findColour (TextEditor::outlineColourId));
    setColour (TextPropertyComponent::backgroundColourId,   findColour (TextEditor::backgroundColourId));
    setColour (TextPropertyComponent::textColourId,         findColour (TextEditor::textColourId));

    setColour (ToggleButton::textColourId, textColor);
    
    // Boolean property comp
    setColour (BooleanPropertyComponent::backgroundColourId,    findColour (TextEditor::backgroundColourId));
    setColour (BooleanPropertyComponent::outlineColourId,       Colours::black);
               
    // Setting Button
    setColour (SettingButton::backgroundColourId, widgetBackgroundColor.brighter());
    setColour (SettingButton::backgroundOnColourId, Colors::toggleOrange);
    setColour (SettingButton::textColourId, Colours::black);
    setColour (SettingButton::textDisabledColourId, Colours::darkgrey);
    
    // Tree View
    setColour (TreeView::selectedItemBackgroundColourId, Colors::elemental.darker (0.6000006f));
    setColour (TreeView::backgroundColourId, LookAndFeel_KV1::backgroundColor);
    
    // Keymap Editor
    setColour (KeyMappingEditorComponent::textColourId, LookAndFeel::textColor);
    setColour (KeyMappingEditorComponent::backgroundColourId, findColour (TreeView::backgroundColourId));

    // Directory Contents Display
    setColour (DirectoryContentsDisplayComponent::textColourId, textColor);
    setColour (DirectoryContentsDisplayComponent::highlightColourId, Colors::elemental.darker (0.6000006f));

    // List Box
    setColour (ListBox::textColourId, textColor);
    
}


// MARK: default sizes

int LookAndFeel::getDefaultScrollbarWidth() { return 12; }

// MARK: Concertina Panel

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
    g.setColour (pc.findColour (PropertyComponent::backgroundColourId));
    g.fillRect (0, 0, width, height - 1);
}

static int getPropertyComponentIndent (PropertyComponent& component)
{
    return jmin (10, component.getWidth() / 10);
}
    
void LookAndFeel::drawPropertyComponentLabel (Graphics& g, int width, int height,
                                              PropertyComponent& component)
{
    ignoreUnused (width);
    
    const auto indent = getPropertyComponentIndent (component);
    
    g.setColour (component.findColour (PropertyComponent::labelTextColourId)
                 .withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.6f));
    
    g.setFont (jmin (height, 24) * 0.65f);
    
    auto r = getPropertyComponentContentPosition (component);
    
    g.drawFittedText (component.getName(),
                      indent, r.getY(), r.getX() - 5, r.getHeight(),
                      Justification::centredLeft, 2);
}

Rectangle<int> LookAndFeel::getPropertyComponentContentPosition (PropertyComponent& component)
{
    const auto textW = jmin (200, component.getWidth() / 2);
    return { textW, 0, component.getWidth() - textW, component.getHeight() - 1 };
}

// MARK: Treeview
void LookAndFeel::drawTreeviewPlusMinusBox (Graphics& g, const Rectangle<float> &area,
                                            Colour backgroundColour, bool isOpen, bool isMouseOver)
{
    LookAndFeel_KV1::drawTreeviewPlusMinusBox (g, area, backgroundColour, isOpen, isMouseOver);
}

}
