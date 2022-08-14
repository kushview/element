/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "engine/midiengine.hpp"
#include "gui/nodes/NodeEditorComponent.h"

namespace Element {

class MidiIONodeEditor : public NodeEditorComponent,
                         public ChangeListener,
                         private Timer
{
public:
    MidiIONodeEditor (const Node& node, MidiEngine& engine, bool ins = true, bool outs = true)
        : NodeEditorComponent (node), midi (engine), showIns (ins), showOuts (outs)
    {
        content.reset (new Content (*this));
        view.setViewedComponent (content.get(), false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
        midi.addChangeListener (this);
        startTimer (1.5 * 1000);
    }

    ~MidiIONodeEditor()
    {
        stopTimer();
        midi.removeChangeListener (this);
        view.setViewedComponent (nullptr, false);
        content.reset();
    }

    void paint (Graphics& g) override
    {
        g.setFont (13.f);
        g.setColour (LookAndFeel::textColor);
        String text = "Host MIDI ";
        if (getNode().isMidiInputNode())
            text << "Input";
        else if (getNode().isMidiOutputNode())
            text << "Output";
        g.drawText (text, getLocalBounds(), Justification::centred);

#if 0
        String text;
        PortArray ins, outs;
        getNode().getPorts (ins, outs, PortType::Audio);
        if (ins.size() > 0 && outs.size() > 0)
        {
            text << ins.size() << " Ins / " << outs.size() << " Outs";
        }
        else if (ins.size() > 0 && outs.size() <= 0)
        {
            text << ins.size() << " Ins";
        }
        else if (ins.size() <= 0 && outs.size() > 0)
        {
            text << ins.size() << " Outs";
        }
        
        if (text.isNotEmpty())
            g.drawText (text, getLocalBounds(), Justification::centred);
#endif
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        content->updateDevices();
        content->updateSize();
    }

    void resized() override
    {
        view.setBounds (getLocalBounds());
        content->setSize (view.getWidth(), content->getHeight());
    }

private:
    MidiEngine& midi;
    bool showIns = true;
    bool showOuts = true;
    Viewport view;
    friend struct Content;

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
                midiOutputs.onChange = [this]() {
                    auto index = midiOutputs.getSelectedItemIndex();
                    if (index == 0)
                    {
                        owner.midi.setDefaultMidiOutput (String());
                    }
                    else if (index > 0)
                    {
                        owner.midi.setDefaultMidiOutput (midiOutputs.getItemText (index));
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
            updateSize();
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
        }

        void updateSize()
        {
            setSize (jmax (getWidth(), 150), computeHeight());
            resized();
        }

        void updateInputs()
        {
            for (auto* btn : midiInputs)
                btn->removeListener (this);
            midiInputs.clearQuick (true);
            for (const auto& name : MidiInput::getDevices())
            {
                auto* toggle = midiInputs.add (new ToggleButton (name));
                toggle->setToggleState (owner.midi.isMidiInputEnabled (name), dontSendNotification);
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

            auto outName = owner.midi.getDefaultMidiOutputName();
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
            owner.midi.setMidiInputEnabled (button->getButtonText(), button->getToggleState());
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

    void timerCallback() override
    {
        if (! content)
            return;
        bool didAnything = false;
        if (showIns && content->midiInputs.size() != MidiInput::getDevices().size())
        {
            content->updateInputs();
            didAnything = true;
        }
        if (showOuts && content->midiOutputs.getNumItems() - 1 != MidiOutput::getDevices().size())
        {
            content->updateOutputs();
            didAnything = true;
        }

        if (didAnything)
            content->updateSize();
    }
};

} // namespace Element
