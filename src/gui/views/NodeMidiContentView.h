
#pragma once

#include "gui/ContentComponent.h"
#include "gui/widgets/MidiChannelSelectComponent.h"

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

    SignalLabel midiProgramLabel;
    struct MidiProgramLayout : public Component
    {
        Slider slider;
        IconButton loadButton;
        IconButton saveButton;

        MidiProgramLayout()
        {
            addAndMakeVisible (slider);
            addAndMakeVisible (loadButton);
            loadButton.setIcon (Icon (getIcons().falSyncAlt, LookAndFeel::textColor));
            addAndMakeVisible (saveButton);
            saveButton.setIcon (Icon (getIcons().fasSave, LookAndFeel::textColor));
        }

        void resized() override
        {
            auto r = getLocalBounds();
            r = r.withWidth (jmax (100 + 48, r.getWidth()));
            slider.setBounds (r.removeFromLeft (100));
            r.removeFromRight (4);
            saveButton.setBounds (r.removeFromRight (20));
            r.removeFromRight (2);
            loadButton.setBounds (r.removeFromRight (20));
        }
    } midiProgram;
    

    SignalLabel midiChannelLabel;
    MidiChannelSelectComponent midiChannel;

    void layoutComponent (Rectangle<int>&, Label&, Component&, int preferedHeight = 0);
    void updateSliders();
    void updateMidiChannels();
};

}
