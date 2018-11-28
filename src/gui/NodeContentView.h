
#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class NodeContentView : public ContentView
{
public:
    NodeContentView();
    ~NodeContentView();

    void resized() override;
    void paint (Graphics& g) override;
    void stabilizeContent() override;

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
};

}
