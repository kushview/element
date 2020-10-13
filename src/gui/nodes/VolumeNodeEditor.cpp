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

#include "gui/nodes/VolumeNodeEditor.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/GuiCommon.h"
#include "gui/NodeIOConfiguration.h"
#include "engine/nodes/VolumeProcessor.h"

namespace Element {

class VolumeNodeEditor::ChannelStrip : public NodeChannelStripComponent,
                                       public AudioProcessorParameter::Listener
{
public:
    ChannelStrip (GuiController& g)
        : NodeChannelStripComponent (g, false)
    { 
        setVolumeMinMax (-30, 12, 0.5);
        
        ioButton = new SettingButton();
        ioButton->setPath (getIcons().fasCog);
        ioButton->onClick = [this]()
        { 
            auto node = getNode();
            GraphNodePtr obj = node.getGraphNode();
            auto* proc = (obj) ? obj->getAudioProcessor() : 0;
            if (! proc) return;

            if (ioButton->getToggleState())
            {
                ioButton->setToggleState (false, dontSendNotification);
                ioBox.clear();
            }
            else
            {
                auto* component = new NodeAudioBusesComponent (node, proc,
                        ViewHelpers::findContentComponent (this));
                auto& box = CallOutBox::launchAsynchronously (
                    std::unique_ptr<Component> (component), 
                    ioButton->getScreenBounds(), 0);
                ioBox.setNonOwned (&box);
            }
        };

        getChannelStrip().addButton (ioButton);

        onVolumeChanged = [this](double value)
        {
            float fvalue = static_cast<float> (value);
            if (param != nullptr)
            {
                param->beginChangeGesture();
                *param = fvalue;
                param->endChangeGesture();
            }
        };
    }

    ~ChannelStrip()
    { 
        if (ioButton)
        {
            ioButton->onClick = nullptr;
            ioButton = nullptr;
        }

        if (param)
            param->removeListener (this);
        param = nullptr;
        onVolumeChanged = nullptr;
    }

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        ignoreUnused (parameterIndex, newValue);
        // updateChannelStrip();
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
    {
        ignoreUnused (parameterIndex, gestureIsStarting);
    }
    
    void updateParameter()
    {
        if (param)
        {
            param->removeListener (this);
            param = nullptr;
        }

        if (auto object = getNode().getGraphNode())
            if (auto* proc = dynamic_cast<VolumeProcessor*> (object->getAudioProcessor()))
                param = dynamic_cast<AudioParameterFloat*> (proc->getParameters()[0]);
        
        stabilizeContent();

        if (param)
            param->addListener (this);
    }

protected:
    float getCurrentVolume() override
    {
        return param != nullptr ? *param : 0.f;
    }

private:
    AudioParameterFloat* param = nullptr;
    SettingButton* ioButton = nullptr;
    OptionalScopedPointer<CallOutBox> ioBox;
};

VolumeNodeEditor::VolumeNodeEditor (const Node& node, GuiController& gui)
    : NodeEditorComponent (node)
{ 
    setOpaque (true);
    strip.reset (new ChannelStrip (gui));
    addAndMakeVisible (strip.get());
    strip->setComboBoxesVisible (false, false);

    setSize (128, 262);
    
    strip->setNode (node);
    strip->updateParameter();
}

VolumeNodeEditor::~VolumeNodeEditor()
{
    strip.reset();
}

void VolumeNodeEditor::paint (Graphics& g) 
{
    g.fillAll (Colours::black);
}

void VolumeNodeEditor::resized()
{
    if (strip)
        strip->setBounds (getLocalBounds());
}

}
