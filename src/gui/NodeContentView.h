
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
private:
    Label nameLabel;
    TextEditor nameEditor;
    Label keyrangeLabel;
    Slider keyrangeSlider;
    Label transposeLabel;
    Slider transposeSlider;

    void layoutComponent (Rectangle<int>&, Label&, Component&);
};

}
