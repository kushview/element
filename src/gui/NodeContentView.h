
#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class NodeContentView : public ContentView,
                        public Slider::Listener
{
public:
    NodeContentView();
    ~NodeContentView();

    void resized() override;
    void paint (Graphics& g) override;
    void stabilizeContent() override;
    void sliderValueChanged (Slider*) override;

private:
    Node node;
    SignalConnection selectedNodeConnection;
    
    Label nameLabel;
    TextEditor nameEditor;
    
    Label keyLowLabel;
    Slider keyLowSlider;

    Label keyHiLabel;
    Slider keyHiSlider;

    Label transposeLabel;
    Slider transposeSlider;

    void layoutComponent (Rectangle<int>&, Label&, Component&);
    void updateSliders();
};

}
