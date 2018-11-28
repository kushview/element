#include "gui/NodeContentView.h"
#include "gui/LookAndFeel.h"

namespace Element {

    NodeContentView::NodeContentView()
    {
        const Font font (12.f);

        addAndMakeVisible (nameLabel);
        nameLabel.setText ("Name", dontSendNotification);
        nameLabel.setFont (font);
        addAndMakeVisible (nameEditor);

        addAndMakeVisible (transposeLabel);
        transposeLabel.setText ("Transpose", dontSendNotification);
        transposeLabel.setFont (font);
        addAndMakeVisible (transposeSlider);
        transposeSlider.setRange (-24, 24, 1);
        transposeSlider.setValue (0);
        transposeSlider.setSliderStyle (Slider::LinearHorizontal);
        transposeSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);

        addAndMakeVisible (keyLowLabel);
        keyLowLabel.setText ("Notes (lower)", dontSendNotification);
        keyLowLabel.setFont (font);
        addAndMakeVisible (keyLowSlider);
        keyLowSlider.setRange (0, 127, 1.0);
        keyLowSlider.setSliderStyle (Slider::LinearHorizontal);
        keyLowSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);
        keyLowSlider.setTextBoxIsEditable (false);
        keyLowSlider.setValue (0);

        addAndMakeVisible (keyHiLabel);
        keyHiLabel.setText ("Notes (upper)", dontSendNotification);
        keyHiLabel.setFont (font);
        addAndMakeVisible (keyHiSlider);
        keyHiSlider.setRange (0, 127, 1.0);
        keyHiSlider.setSliderStyle (Slider::LinearHorizontal);
        keyHiSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);
        keyHiSlider.setTextBoxIsEditable (false);
        keyHiSlider.setValue (127);
    }

    NodeContentView::~NodeContentView()
    {

    }

    void NodeContentView::paint (Graphics& g)
    {
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void NodeContentView::resized()
    {
        auto r (getLocalBounds().reduced (2));
        layoutComponent (r, nameLabel, nameEditor);
        layoutComponent (r, transposeLabel, transposeSlider);
        layoutComponent (r, keyLowLabel, keyLowSlider);
        layoutComponent (r, keyHiLabel, keyHiSlider);
    }

    void NodeContentView::layoutComponent (Rectangle<int>& r, Label& l, Component& c)
    {
        static const int settingHeight = 20;
        static const int labelWidth = 80;
        static const int spacing = 8;
        auto r2 = r.removeFromTop (settingHeight);
        l.setBounds (r2.removeFromLeft (labelWidth));
        c.setBounds (r2);
        r.removeFromTop (spacing);
    }
}
