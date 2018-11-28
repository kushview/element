
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/LookAndFeel.h"
#include "gui/NodeContentView.h"
#include "gui/ViewHelpers.h"

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

        keyLowSlider.addListener (this);
        keyHiSlider.addListener (this);
        transposeSlider.addListener (this);
    }

    NodeContentView::~NodeContentView()
    {
        keyLowSlider.removeListener (this);
        keyHiSlider.removeListener (this);
        transposeSlider.removeListener (this);
        selectedNodeConnection.disconnect();
    }

    void NodeContentView::paint (Graphics& g)
    {
        g.fillAll (Element::LookAndFeel::backgroundColor);
    }

    void NodeContentView::resized()
    {
        auto r (getLocalBounds().reduced (2));
        r.removeFromTop (4);
        r.removeFromRight (4);
        layoutComponent (r, nameLabel, nameEditor);
        layoutComponent (r, keyLowLabel, keyLowSlider);
        layoutComponent (r, keyHiLabel, keyHiSlider);
        layoutComponent (r, transposeLabel, transposeSlider);
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

    void NodeContentView::stabilizeContent()
    {
        auto *cc = ViewHelpers::findContentComponent(this);
        jassert(cc);
        auto& gui = *cc->getAppController().findChild<GuiController>();
        if (! selectedNodeConnection.connected())
            selectedNodeConnection = gui.nodeSelected.connect (std::bind (
                &NodeContentView::stabilizeContent, this));
        node = gui.getSelectedNode();
        nameEditor.getTextValue().referTo (node.getPropertyAsValue (Tags::name));

        if (GraphNodePtr object = node.getGraphNode())
        {
            const auto range (object->getKeyRange());
            keyLowSlider.setValue ((double) range.getStart(), dontSendNotification);
            keyHiSlider.setValue ((double) range.getEnd(), dontSendNotification);
            transposeSlider.setValue ((double) object->getTransposeOffset(), dontSendNotification);
        }

        DBG("[EL] update node content view for " << node.getName());
    }

    void NodeContentView::sliderValueChanged (Slider* slider)
    {
        GraphNodePtr object = node.getGraphNode();
        if (object == nullptr)
            return;
        
        if (slider == &keyLowSlider)
        {
            auto keyRange (object->getKeyRange());
            keyRange.setStart (roundToInt (slider->getValue()));
            object->setKeyRange (keyRange);
        }
        else if (slider == &keyHiSlider)
        {
            auto keyRange (object->getKeyRange());
            keyRange.setEnd (roundToInt (slider->getValue()));
            object->setKeyRange (keyRange);
        }
        else if (slider == &transposeSlider)
        {
            object->setTransposeOffset (roundToInt (slider->getValue()));
        }
        
        ValueTree data (node.getValueTree());
        auto range (object->getKeyRange());
        data.setProperty ("keyRangeStart", range.getStart(), nullptr)
            .setProperty ("keyRangeEnd", range.getEnd(), nullptr)
            .setProperty ("transposeOffset", object->getTransposeOffset(), nullptr);
    }
}
