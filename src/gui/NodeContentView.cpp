#include "gui/NodeContentView.h"
#include "gui/LookAndFeel.h"

namespace Element {

    NodeContentView::NodeContentView()
    {
        addAndMakeVisible (nameLabel);
        nameLabel.setText ("Name", dontSendNotification);
        addAndMakeVisible (nameEditor);

        addAndMakeVisible (transposeLabel);
        transposeLabel.setText ("Transpose", dontSendNotification);
        addAndMakeVisible (transposeSlider);
        transposeSlider.setRange (-24, 24, 1);
        transposeSlider.setValue (0);
        transposeSlider.setSliderStyle (Slider::LinearHorizontal);
        transposeSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);

        addAndMakeVisible (keyrangeLabel);
        keyrangeLabel.setText ("Key Range", dontSendNotification);
        addAndMakeVisible (keyrangeSlider);
        keyrangeSlider.setRange (0, 127, 1.0);
        keyrangeSlider.setSliderStyle (Slider::ThreeValueHorizontal);
        keyrangeSlider.setTextBoxStyle (Slider::TextBoxRight, true, 40, 18);
        keyrangeSlider.setTextBoxIsEditable (false);
        keyrangeSlider.setMinAndMaxValues (0, 127, dontSendNotification);
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
        layoutComponent (r, keyrangeLabel, keyrangeSlider);
    }

    void NodeContentView::layoutComponent (Rectangle<int>& r, Label& l, Component& c)
    {
        static const int settingHeight = 20;
        static const int labelWidth = 80;
        static const int spacing = 4;
        auto r2 = r.removeFromTop (settingHeight);
        l.setBounds (r2.removeFromLeft (labelWidth));
        c.setBounds (r2);
        r.removeFromTop (spacing);
    }
}
