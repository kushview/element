
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
    struct MidiProgramLayout : public Component
    {
        Label name;
        Slider slider;
        IconButton loadButton;
        IconButton saveButton;
        IconButton globalButton;
        IconButton powerButton;
        IconButton trashButton;

        MidiProgramLayout()
        {
            addAndMakeVisible (name);

            addAndMakeVisible (slider);
            slider.setSliderStyle (Slider::IncDecButtons);
            slider.setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
            slider.setRange (1.0, 128.0, 1.0);

            addAndMakeVisible (loadButton);
            loadButton.setIcon (Icon (getIcons().farRedoAlt, LookAndFeel::textColor), 11.6f);
            addAndMakeVisible (saveButton);
            saveButton.setIcon (Icon (getIcons().farSave, LookAndFeel::textColor));
            addAndMakeVisible (trashButton);
            trashButton.setIcon (Icon (getIcons().farTrashAlt, LookAndFeel::textColor));

            addAndMakeVisible (globalButton);
            globalButton.setTooltip ("Use global MIDI programs");
            globalButton.setColour (TextButton::buttonOnColourId, Colors::toggleGreen);
            globalButton.setClickingTogglesState (true);
            globalButton.setIcon (Icon (getIcons().farGlobe, LookAndFeel::textColor));

            addAndMakeVisible (powerButton);
            powerButton.setTooltip ("Enable/disable MIDI programs");
            powerButton.setColour (TextButton::buttonOnColourId, Colors::toggleBlue);
            powerButton.setClickingTogglesState (true);
            powerButton.setIcon (Icon (getIcons().fasPowerOff, LookAndFeel::textColor));
        }

        void resized() override
        {
            auto r = getLocalBounds();
            auto r2 = r.removeFromTop (r.getHeight() / 2);
            r = r.withWidth (jmax (100 + 48, r.getWidth()));
            powerButton.setBounds (r.removeFromRight (20));
            r.removeFromRight (1);
            globalButton.setBounds (r.removeFromRight (20));
            r.removeFromRight (1);
            trashButton.setBounds (r.removeFromRight (20));
            r.removeFromRight (1);
            loadButton.setBounds (r.removeFromRight (20));
            r.removeFromRight (1);
            saveButton.setBounds (r.removeFromRight (20));
            slider.setBounds (r.removeFromLeft (108));

            name.setBounds (r2);
        }
    } midiProgram;
    

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
