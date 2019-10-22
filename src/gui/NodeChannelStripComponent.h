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

#include "ElementApp.h"
#include "controllers/GuiController.h"
#include "engine/GraphNode.h"
#include "gui/ChannelStripComponent.h"
#include "Signals.h"

namespace Element {

class NodeChannelStripComponent : public Component,
                                  public Timer,
                                  public ComboBox::Listener,
                                  private Value::Listener
{
public:
    std::function<void()> onNodeChanged;
    NodeChannelStripComponent (GuiController& g, bool handleNodeSelected = true)
        : gui (g), listenForNodeSelected (handleNodeSelected)
    {
        addAndMakeVisible (channelStrip);
        addAndMakeVisible (nodeName);
        nodeName.setText ("", dontSendNotification);
        nodeName.setJustificationType (Justification::centredBottom);
        nodeName.setEditable (false, true, false);
        nodeName.setFont (10.f);
        nodeName.onTextChange = [this]
        {
            if (node.isValid())
                node.setProperty (Tags::name, nodeName.getText());
        };

        addAndMakeVisible (channelBox);
        channelBox.setJustificationType (Justification::centred);

        addAndMakeVisible (flowBox);
        flowBox.setJustificationType (Justification::centred);

        bindSignals();
    }

    ~NodeChannelStripComponent()
    {
        unbindSignals();
    }

    ChannelStripComponent& getChannelStrip() { return channelStrip; }
    
    void setVolumeMinMax (double minDb, double maxDb, double skew = 2.0)
    {
        channelStrip.setMinMaxDecibels (minDb, maxDb);
        channelStrip.setFaderSkewFactor (skew);
    }

    void bindSignals()
    {
        unbindSignals();
        displayName.addListener (this);
        flowBox.addListener (this);
        if (listenForNodeSelected)
            nodeSelectedConnection = gui.nodeSelected.connect (
                std::bind (&NodeChannelStripComponent::nodeSelected, this));
        volumeChangedConnection = channelStrip.volumeChanged.connect (
            std::bind (&NodeChannelStripComponent::volumeChanged, this, std::placeholders::_1));
        powerChangedConnection = channelStrip.powerChanged.connect (
            std::bind (&NodeChannelStripComponent::powerChanged, this));
        muteChangedConnection = channelStrip.muteChanged.connect (
            std::bind (&NodeChannelStripComponent::muteChanged, this));
        volumeDoubleClickedConnection = channelStrip.volumeLabelDoubleClicked.connect (
            std::bind (&NodeChannelStripComponent::setUnityGain, this));
    }

    void unbindSignals()
    {
        displayName.removeListener (this);
        flowBox.removeListener (this);
        nodeSelectedConnection.disconnect();
        volumeChangedConnection.disconnect();
        powerChangedConnection.disconnect();
        muteChangedConnection.disconnect();
        volumeDoubleClickedConnection.disconnect();
    }

    void resized() override
    {
        auto r (getLocalBounds());
        nodeName.setBounds (r.removeFromTop(22).reduced (2));
        r.removeFromTop (10); // padding between strip title and IO boxes

        auto r2 = r.removeFromBottom (jmin (268, r.getHeight()));
        int boxSize = r2.getWidth() - 8;
        flowBox.setBounds (r2.removeFromTop (16).withSizeKeepingCentre (boxSize, 14));
        channelBox.setBounds (r2.removeFromTop(16).withSizeKeepingCentre (boxSize, 14));
        channelStrip.setBounds (r2);
    }

    inline virtual void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel::widgetBackgroundColor);
        g.fillAll();
        g.setColour (LookAndFeel::contentBackgroundColor);
        g.drawLine (getWidth() - 1.f, 0.0, getWidth() - 1.f, getHeight());
    }

    inline void timerCallback() override
    {
        auto& meter = channelStrip.getDigitalMeter();
        const bool isAudioOut = node.isAudioIONode();
        if (GraphNodePtr ptr = node.getGraphNode())
        {
            const int startChannel = jmax (0, channelBox.getSelectedId() - 1);
            if (ptr->getNumAudioOutputs() == 1)
            {
                if (isAudioOutNode)
                    for (int c = 0; c < 2; ++c)
                        meter.setValue (c, ptr->getInputRMS (startChannel));
                else
                    for (int c = 0; c < 2; ++c)
                        meter.setValue (c, ptr->getOutputRMS (startChannel));
            }
            else
            {
                const int endChannel = startChannel + 2;
                if (isAudioOutNode || isMonitoringInputs())
                    for (int c = startChannel; c < endChannel; ++c)
                        meter.setValue (c - startChannel, ptr->getInputRMS (c));
                else
                    for (int c = startChannel; c < endChannel; ++c)
                        meter.setValue (c - startChannel, ptr->getOutputRMS (c));
            }

            const auto cv = getCurrentVolume();
            if (static_cast<float> (channelStrip.getVolume()) != cv)
                channelStrip.setVolume (cv, dontSendNotification);
            
            channelStrip.setPower (! ptr->isSuspended(), false);
            if (channelStrip.isMuted() != ptr->isMuted())
                channelStrip.setMuted (ptr->isMuted(), false);
        }
        else
        {
            meter.resetPeaks();
            stopTimer();
        }

        meter.refresh();
    }

    inline void stabilizeContent()
    {
        updateComboBoxes();
        updateNodeName();
        updateChannelStrip();
    }

    inline void updateChannelStrip()
    {
        if (GraphNodePtr object = node.getGraphNode())
        {
            SharedConnectionBlock b1 (volumeChangedConnection);
            SharedConnectionBlock b2 (powerChangedConnection);
            SharedConnectionBlock b3 (muteChangedConnection);

            channelStrip.setVolume (getCurrentVolume());
            channelStrip.setPower (! object->isSuspended());
            channelStrip.setMuted (object->isMuted(), false);

            b1.unblock();
            b2.unblock();
            b3.unblock();
        }
    }

    inline void setNodeNameEditable (const bool isEditable)
    {
        nodeName.setEditable (false, isEditable, false);
    }

    inline void setNode (const Node& newNode)
    {
        stopTimer();
        node = newNode;
        isAudioOutNode = node.isAudioOutputNode();
        isAudioInNode  = node.isAudioInputNode();
        audioIns.clearQuick(); audioOuts.clearQuick();
        node.getPorts (audioIns, audioOuts, PortType::Audio);
        displayName.referTo (node.getPropertyAsValue (Tags::name));
        stabilizeContent();
        startTimerHz (meterSpeedHz);

        if (onNodeChanged)
            onNodeChanged();
    }

    inline Node getNode() const { return node; }
    
    inline void setComboBoxesVisible (bool showChannelBox = true, bool showFlowBox = true)
    {
        useChannelBox = showChannelBox;
        useFlowBox = showFlowBox;
        updateComboBoxes();
        resized();
    }

    /** @internal */
    inline void comboBoxChanged (ComboBox* box) override
    {    
        if (box == &flowBox)
        {
            updateComboBoxes (false, true);
            updateChannelStrip();
        }
    }

    /** Called when the volume slider changes. If this is set, you probably
        also want to override getCurrentVolume() */
    std::function<void(double)> onVolumeChanged;

protected:
    /** Override this to return volume from the backend/model layer. The
        default returns either the input or output gain of the node */
    virtual float getCurrentVolume()
    {
        GraphNodePtr object = node.getGraphNode();
        if (object == nullptr)
            return 0.f;

        float gain = isMonitoringInputs() || isAudioOutNode ? object->getInputGain() : object->getGain();
        return Decibels::gainToDecibels (gain, -60.f);
    }

private:
    friend class NodeChannelStripView;
    GuiController& gui;
    Label nodeName;
    Node node;
    PortArray audioIns, audioOuts;
    ComboBox channelBox, flowBox;
    ChannelStripComponent channelStrip;
    bool listenForNodeSelected;
    
    bool useFlowBox = true;
    bool useChannelBox = true;

    int meterSpeedHz    = 15;
    bool isAudioOutNode = false;
    bool isAudioInNode  = false;
    bool monoMeter      = false;

    Value displayName;

    SignalConnection nodeSelectedConnection;
    SignalConnection volumeChangedConnection;
    SignalConnection powerChangedConnection;
    SignalConnection volumeDoubleClickedConnection;
    SignalConnection muteChangedConnection;

    inline bool isMonitoringInputs() const  { return flowBox.getSelectedId() == 1; }
    inline bool isMonitoringOutputs() const { return flowBox.getSelectedId() == 2; }

    void valueChanged (Value& value) override
    {
        if (value.refersToSameSourceAs (displayName))
            updateNodeName();
    }

    void updateNodeName()
    {
        if (node.isValid())
        {
            nodeName.setText (node.getDisplayName(), dontSendNotification);
            String tooltip = node.getDisplayName();
            if (node.hasModifiedName())
                tooltip << " (" << node.getPluginName() << ")";
            nodeName.setTooltip (tooltip);
        }
    }

    void updateComboBoxes (bool doFlowBox = true, bool doChannelBox = true)
    {
        if (doFlowBox)
        {
            int flowId = flowBox.getSelectedId();
            if (flowId <= 0)
                flowId = 2;
            flowBox.clear();
            flowBox.setTooltip ("Signal flow to monitor");
            
            if (audioIns.size() > 0)
                flowBox.addItem ("Input", 1);
            if (audioOuts.size() > 0)
                flowBox.addItem ("Output", 2);
        
            if (flowBox.getNumItems() > 0 && !isAudioInNode && !isAudioOutNode)
            {
                flowBox.setVisible (useFlowBox);
                flowBox.setSelectedId (flowId, false);
                if (flowBox.getSelectedId() <= 0)
                    flowBox.setSelectedItemIndex (0);
            }
            else
            {
                flowBox.setVisible (false);
            }
        }

        if (doChannelBox)
        {
            channelBox.setTooltip ("Channel(s) to monitor");
            channelBox.clear();

            // audio out node ports flipped until more robust
            auto& ports = isAudioOutNode ? audioIns :        
                    isMonitoringInputs() ? audioIns : audioOuts;
            
            const bool monoPorts = ports.size() % 2 != 0;
            const int step = monoPorts ? 1 : 2;
            
            for (int i = 0; i < ports.size(); i += step)
            {
                String text (int (i + 1));
                if (! monoPorts)
                    text << " - " << int (i + 2);
                channelBox.addItem (text, i + 1);
            }

            if (channelBox.getNumItems() > 0)
            {
                channelBox.setVisible (useFlowBox);
                channelBox.setSelectedItemIndex (0);
            }
            else
            {
                channelBox.setVisible (false);
            }
        }
    }

    void nodeSelected()
    {
        setNode (gui.getSelectedNode());
    }

    void powerChanged()
    {
        if (node.isValid())
            node.setProperty (Tags::bypass, ! channelStrip.isPowerOn());
        if (auto* obj = node.getGraphNode())
            obj->suspendProcessing (! channelStrip.isPowerOn());
    }

    void volumeChanged (double value)
    {
        if (onVolumeChanged != nullptr)
            return onVolumeChanged (value);
        
        if (GraphNodePtr object = node.getGraphNode())
        {
            auto gain = Decibels::decibelsToGain (value, -60.0);
            if (isAudioOutNode || isMonitoringInputs())
            {
                node.setProperty ("inputGain", gain);
                object->setInputGain (static_cast<float> (gain));
            }
            else
            {
                node.setProperty ("gain", gain);
                object->setGain (static_cast<float> (gain));
            }
        }
    }

    void muteChanged()
    {
        if (node.isValid())
            node.setProperty (Tags::mute, channelStrip.isMuted());
        if (auto* obj = node.getGraphNode())
            obj->setMuted (channelStrip.isMuted());
    }

    void setUnityGain()
    {
        channelStrip.setVolume (0.0);
    }
};

}
