#pragma once

#include "gui/nodes/NodeEditorComponent.h"
#include "session/DeviceManager.h"

namespace Element {

class AudioIONodeEditor : public NodeEditorComponent,
                         public ChangeListener
{
public:
    AudioIONodeEditor (const Node& node, DeviceManager& devs, bool ins = true, bool outs = true)
        : NodeEditorComponent (node), devices (devs), showIns(ins), showOuts(outs)
    {
       #if ! EL_RUNNING_AS_PLUGIN
        content.reset (new Content (*this));
        view.setViewedComponent (content.get(), false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
        devices.addChangeListener (this);
       #endif
    }

    ~AudioIONodeEditor()
    {
       #if ! EL_RUNNING_AS_PLUGIN
        devices.removeChangeListener (this);
        view.setViewedComponent (nullptr, false);
        content.reset();
       #endif
    }
    
    void paint (Graphics& g) override
    {
       #if EL_RUNNING_AS_PLUGIN
        g.setFont (13.f);
        g.setColour (LookAndFeel::textColor);
        String text = "Host Audio ";
        if (getNode().isAudioInputNode())
            text << "Input";
        else if (getNode().isAudioOutputNode())
            text << "Output";
        g.drawText (text, getLocalBounds(), Justification::centred);
       #endif
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
       #if ! EL_RUNNING_AS_PLUGIN
        content->updateDevices();
       #endif
    }

    void resized() override
    {
       #if ! EL_RUNNING_AS_PLUGIN
        view.setBounds (getLocalBounds());
        content->setSize (view.getWidth(), content->getHeight());
       #endif
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
        Content (AudioIONodeEditor& ed) 
            : owner (ed)
        {
            if (owner.showOuts)
            {
                
            }

            if (owner.showIns)
            {   
                
            }

            updateDevices();
        }
        
        int computeHeight()
        {
            return 100;
        }

        void updateDevices()
        {
            if (owner.showIns)
                updateInputs();
            if (owner.showOuts)
                updateOutputs();
        }

        void updateInputs()
        {
            
        }

        void updateOutputs()
        {
            
        }

        void buttonClicked (Button* button) override
        {
        }

        void resized() override
        {
            
        }
        
        AudioIONodeEditor& owner;
    };

    std::unique_ptr<Content> content;
};

}
