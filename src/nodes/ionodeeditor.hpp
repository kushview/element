// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/ui/nodeeditor.hpp>
#include <element/ui/style.hpp>

#include <element/devices.hpp>

namespace element {

class AudioIONodeEditor : public NodeEditor,
                          public ChangeListener
{
public:
    AudioIONodeEditor (const Node& node, DeviceManager& devs, bool ins = true, bool outs = true)
        : NodeEditor (node), devices (devs), showIns (ins), showOuts (outs)
    {
        content.reset (new Content (*this));
        view.setViewedComponent (content.get(), false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
        devices.addChangeListener (this);
    }

    ~AudioIONodeEditor()
    {
        devices.removeChangeListener (this);
        view.setViewedComponent (nullptr, false);
        content.reset();
    }

    void paint (Graphics& g) override
    {
        g.setFont (13.f);
        g.setColour (Colors::textColor);
        String text = "Host Audio ";
        if (getNode().isAudioInputNode())
            text << "Input";
        else if (getNode().isAudioOutputNode())
            text << "Output";
        g.drawText (text, getLocalBounds(), Justification::centred);
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

    friend struct Content;
    std::unique_ptr<Content> content;
};

} // namespace element
