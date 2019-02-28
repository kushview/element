#pragma once

#include "gui/nodes/NodeEditorComponent.h"
#include "session/DeviceManager.h"

namespace Element {

class MidiIONodeEditor : public NodeEditorComponent
{
public:
    MidiIONodeEditor (const Node& node, DeviceManager& devs, bool ins = true, bool outs = true)
        : NodeEditorComponent (node), devices (devs), showIns(ins), showOuts(outs)
    {
        content.reset (new Content (*this));
        view.setViewedComponent (content.get(), false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
    }

    ~MidiIONodeEditor()
    {
        view.setViewedComponent (nullptr, false);
        content.reset();
    }
    
    void resized() override
    {
        view.setBounds (getLocalBounds());
        content->setSize (view.getWidth(), content->getHeight());
    }

private:
    DeviceManager& devices;
    bool showIns = true;
    bool showOuts = true;
    Viewport view;
    friend class Content;

    struct Content : public Component,
                     public Button::Listener
    {
        Content (MidiIONodeEditor& ed) 
            : owner (ed)
        {
            int height = 10;
            if (owner.showOuts)
            {
                addAndMakeVisible (midiOutputLabel);
                midiOutputLabel.setText ("MIDI Output", dontSendNotification);
                midiOutputLabel.setJustificationType (Justification::centredLeft);
                addAndMakeVisible (midiOutputs);
                height += 22;
            }

            if (owner.showIns)
            {   
                if (owner.showOuts)
                    height += 10;
                
                addAndMakeVisible (midiInputLabel);
                midiInputLabel.setText ("MIDI Inputs", dontSendNotification);
                midiInputLabel.setJustificationType (Justification::centredLeft);
                height += 22;
                
                for (const auto& name : MidiInput::getDevices())
                {
                    auto* toggle = midiInputs.add (new ToggleButton (name));
                    toggle->setToggleState (owner.devices.isMidiInputEnabled (name), dontSendNotification);
                    
                    addAndMakeVisible (toggle);
                    height += 22;
                }
            }

            setSize (200, height);
        }

        void buttonClicked (Button* button)
        {
            DBG("device: " << button->getButtonText());
        }

        void resized() override
        {
            auto r = getLocalBounds().withWidth (jmax (getWidth(), 200));
            r.removeFromTop (10);
            auto leftSlice = r.removeFromLeft (100);
            auto rightSlice = r;
            rightSlice.removeFromRight (14);

            if (midiOutputLabel.isVisible())
            {
                midiOutputLabel.setBounds (leftSlice.removeFromTop (22));
                midiOutputs.setBounds (rightSlice.removeFromTop (22));
            }
            
            leftSlice.removeFromTop (10);
            rightSlice.removeFromTop (10);

            if (midiInputLabel.isVisible())
            {
                midiInputLabel.setBounds (leftSlice.removeFromTop (22));
                for (auto* input : midiInputs)
                    input->setBounds (rightSlice.removeFromTop (22));
            }
        }
        
        MidiIONodeEditor& owner;
        Label midiOutputLabel;
        ComboBox midiOutputs;
        Label midiInputLabel;
        OwnedArray<ToggleButton> midiInputs;
    };

    std::unique_ptr<Content> content;
};

}