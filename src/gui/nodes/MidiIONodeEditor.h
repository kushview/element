#pragma once

#include "gui/nodes/NodeEditorComponent.h"
#include "session/DeviceManager.h"

namespace Element {

class MidiIONodeEditor : public NodeEditorComponent,
                         public ChangeListener
{
public:
    MidiIONodeEditor (const Node& node, DeviceManager& devs, bool ins = true, bool outs = true)
        : NodeEditorComponent (node), devices (devs), showIns(ins), showOuts(outs)
    {
        content.reset (new Content (*this));
        view.setViewedComponent (content.get(), false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
        devices.addChangeListener (this);
    }

    ~MidiIONodeEditor()
    {
        devices.removeChangeListener (this);
        view.setViewedComponent (nullptr, false);
        content.reset();
    }
    
    void changeListenerCallback (ChangeBroadcaster*) override
    {
        content->updateDevices();
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
            if (owner.showOuts)
            {
                addAndMakeVisible (midiOutputLabel);
                midiOutputLabel.setText ("MIDI Output", dontSendNotification);
                midiOutputLabel.setJustificationType (Justification::centredLeft);
                midiOutputLabel.setFont (Font (12.f));
                addAndMakeVisible (midiOutputs);
                midiOutputs.onChange = [this]()
                {
                    auto index = midiOutputs.getSelectedItemIndex();
                    if (index == 0)
                    {
                        owner.devices.setDefaultMidiOutput (String());
                    }
                    else if (index > 0)
                    {
                        owner.devices.setDefaultMidiOutput (midiOutputs.getItemText (index));
                    }
                };
            }

            if (owner.showIns)
            {   
                addAndMakeVisible (midiInputLabel);
                midiInputLabel.setText ("MIDI Inputs", dontSendNotification);
                midiInputLabel.setJustificationType (Justification::centredLeft);
                midiInputLabel.setFont (Font (12.f));
            }

            updateDevices();
        }
        
        int computeHeight()
        {
            int height = 10;
            if (owner.showOuts)
                height += 44;

            if (owner.showIns)
            {   
                if (owner.showOuts)
                    height += 10;
                height += 22;
            }
            height += midiInputs.size() * 22;
            return height;
        }

        void updateDevices()
        {
            if (owner.showIns)
                updateInputs();
            if (owner.showOuts)
                updateOutputs();
            setSize (jmax (getWidth(), 150), computeHeight());
            resized();
        }

        void updateInputs()
        {
            for (auto* btn : midiInputs)
                btn->removeListener (this);
            midiInputs.clearQuick(true);
            for (const auto& name : MidiInput::getDevices())
            {
                auto* toggle = midiInputs.add (new ToggleButton (name));
                toggle->setToggleState (owner.devices.isMidiInputEnabled (name), dontSendNotification);
                toggle->addListener (this);
                addAndMakeVisible (toggle);
            }
        }

        void updateOutputs()
        {
            midiOutputs.clear (dontSendNotification);
            int itemId = 1;
            midiOutputs.addItem ("<< none >>", itemId++);
            for (const auto& name : MidiOutput::getDevices())
            {
                midiOutputs.addItem (name, itemId++);
            }

            auto outName = owner.devices.getDefaultMidiOutputName();
            if (outName.isEmpty())
            {
                midiOutputs.setSelectedItemIndex (0, dontSendNotification);
                return;
            }

            for (int i = 0; i < midiOutputs.getNumItems(); ++i)
            {
                if (outName == midiOutputs.getItemText (i))
                {
                    midiOutputs.setSelectedItemIndex (i, dontSendNotification);
                    break;
                }
            }
        }

        void buttonClicked (Button* button) override
        {
            owner.devices.setMidiInputEnabled (button->getButtonText(), button->getToggleState());
        }

        void resized() override
        {
            auto r = getLocalBounds().withWidth (jmax (getWidth(), 200)).reduced (10, 0);
            r.removeFromTop (10);
            
            if (midiOutputLabel.isVisible())
            {
                midiOutputLabel.setBounds (r.removeFromTop (22));
                midiOutputs.setBounds (r.removeFromTop (22));
            }
            
            if (owner.showOuts && owner.showIns)
                r.removeFromTop (10);

            if (midiInputLabel.isVisible())
            {
                midiInputLabel.setBounds (r.removeFromTop (22));
                for (auto* input : midiInputs)
                    input->setBounds (r.removeFromTop (22));
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