
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/MidiChannelSelectComponent.h"
#include "gui/widgets/NodeMidiProgramComponent.h"

namespace Element {

class NodeMidiContentView : public ContentView,
                        public Slider::Listener
{
public:
    NodeMidiContentView();
    ~NodeMidiContentView();

    void stabilizeContent() override;

    void resized() override;
    void paint (Graphics& g) override;
    void sliderValueChanged (Slider*) override;

private:
    Node node;
    SignalConnection selectedNodeConnection;
    SignalConnection midiProgramChangedConnection;

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
    
    SignalLabel keyLowLabel;
    Slider keyLowSlider;

    SignalLabel keyHiLabel;
    Slider keyHiSlider;

    SignalLabel transposeLabel;
    Slider transposeSlider;

    SignalLabel midiProgramLabel;
    NodeMidiProgramComponent midiProgram;
    
    SignalLabel midiChannelLabel;
    MidiChannelSelectComponent midiChannel;

    PropertyPanel props;
    void updateProperties();

    void layoutComponent (Rectangle<int>&, Label&, Component&, int preferedHeight = 0);
    void updateSliders();
    void updateMidiChannels();
    void updateMidiProgram();
};

}
