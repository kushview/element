
#include "LookAndFeel.h"
#include "gui/Buttons.h"
#include "gui/widgets/MidiBlinker.h"
#include "gui/ActivationDialog.h"

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
    
    // MIDI Blinkers
    setColour (MidiBlinker::backgroundColourId, findColour (SettingButton::backgroundColourId));
    setColour (MidiBlinker::outlineColourId, LookAndFeel::widgetBackgroundColor.brighter().brighter());

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

    // Slider
    setColour (Slider::thumbColourId,               Colours::black);
    setColour (Slider::textBoxTextColourId,         LookAndFeel::textColor);
    setColour (Slider::trackColourId,               Colours::black);
    setColour (Slider::textBoxBackgroundColourId,   findColour (TextEditor::backgroundColourId));
    setColour (Slider::textBoxHighlightColourId,    findColour (TextEditor::highlightColourId));
    setColour (Slider::textBoxOutlineColourId,      findColour (TextEditor::outlineColourId));
    setColour (Slider::textBoxTextColourId,         findColour (TextEditor::textColourId));

    // Hyperlink button
    setColour (HyperlinkButton::textColourId, Colors::toggleBlue);

    // DockItem
    setColour (DockItem::selectedHighlightColourId, Colors::toggleBlue);

    // ProgressBar
    setColour (ProgressBar::foregroundColourId, Colors::elemental);
    setColour (ProgressBar::backgroundColourId, findColour (
        DocumentWindow::backgroundColourId).darker());
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

// ProgressBar

void LookAndFeel::drawLinearProgressBar (Graphics& g, ProgressBar& progressBar,
                                         int width, int height,
                                         double progress, const String& textToShow)
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
            p.addQuadrilateral (x, 0.0f,
                                x + stripeWidth * 0.5f, 0.0f,
                                x, static_cast<float> (height),
                                x - stripeWidth * 0.5f, static_cast<float> (height));

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
void LookAndFeel::drawProgressBar (Graphics& g, ProgressBar& progressBar,
                                   int width, int height, double progress, const String& textToShow)
{
    String theText = textToShow;
    if (auto* const trialBar = dynamic_cast<TrialDaysProgressBar*> (&progressBar))
    {
        const double elapsed = trialBar->periodDays * progress;
        const double remains = trialBar->periodDays - elapsed;
        if (progress >= 1.0)
        {
            theText = "Trial Expired";
        }
        else
        {
            theText = "Trial expires in ";
            theText << RelativeTime::days(remains).getDescription();
        }
    }
    drawLinearProgressBar (g, progressBar, width, height, progress, theText);
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

void LookAndFeel::drawLinearSliderBackground (Graphics& g, int x, int y, int width, int height,
                                              float /*sliderPos*/,
                                              float /*minSliderPos*/,
                                              float /*maxSliderPos*/,
                                              const Slider::SliderStyle /*style*/, 
                                              Slider& slider)
{
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
}

}
