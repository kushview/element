
#pragma once

#include "gui/ContentComponent.h"

namespace Element {

class NodeContentView : public ContentView,
                        public Slider::Listener
{
public:
    NodeContentView();
    ~NodeContentView();

    void stabilizeContent() override;

    void resized() override;
    void paint (Graphics& g) override;
    void sliderValueChanged (Slider*) override;

private:
    Node node;
    SignalConnection selectedNodeConnection;
    
    class SignalLabel : public Label
    {
    public:
        SignalLabel() { }
        ~SignalLabel() { }
        
        inline void mouseDoubleClick (const MouseEvent& ev) override
        { 
            if (onDoubleClicked)
                onDoubleClicked (ev);
        }

        std::function<void(const MouseEvent&)> onDoubleClicked;
    };

    Label nameLabel;
    TextEditor nameEditor;
    
    SignalLabel keyLowLabel;
    Slider keyLowSlider;

    SignalLabel keyHiLabel;
    Slider keyHiSlider;

    SignalLabel transposeLabel;
    Slider transposeSlider;

    void layoutComponent (Rectangle<int>&, Label&, Component&);
    void updateSliders();
};

}
