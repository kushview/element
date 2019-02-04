
#include "ElementApp.h"
#include "controllers/GuiController.h"
#include "engine/GraphNode.h"
#include "gui/ChannelStripComponent.h"
#include "Signals.h"

namespace Element {

class NodeChannelStripComponent : public Component,
                                  public Timer,
                                  public ComboBox::Listener
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

    void bindSignals()
    {
        unbindSignals();
        flowBox.addListener (this);
        if (listenForNodeSelected)
            nodeSelectedConnection = gui.nodeSelected.connect (
                std::bind (&NodeChannelStripComponent::nodeSelected, this));
        volumeChangedConnection = channelStrip.volumeChanged.connect (
            std::bind (&NodeChannelStripComponent::volumeChanged, this, std::placeholders::_1));
        powerChangedConnection = channelStrip.powerChanged.connect (
            std::bind (&NodeChannelStripComponent::powerChanged, this));
        volumeDoubleClickedConnection = channelStrip.volumeLabelDoubleClicked.connect (
            std::bind (&NodeChannelStripComponent::setUnityGain, this));
    }

    void unbindSignals()
    {
        flowBox.removeListener (this);
        nodeSelectedConnection.disconnect();
        volumeChangedConnection.disconnect();
        powerChangedConnection.disconnect();
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
        const bool isAudioOut = node.isAudioIONode ();
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
            channelStrip.setPower (!ptr->isSuspended(), false);
        }
        else
        {
            meter.resetPeaks();
            stopTimer();
        }

        meter.refresh();
        meter.repaint();
    }

    inline void stabilizeContent()
    {
        updateComboBoxes();

        if (node.isValid())
        {
            nodeName.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
            nodeName.setTooltip (node.getName());
        }

        updateChannelStrip();
    }

    inline void updateChannelStrip()
    {
        if (GraphNodePtr object = node.getGraphNode())
        {
            SharedConnectionBlock b1 (volumeChangedConnection);
            SharedConnectionBlock b2 (powerChangedConnection);
            float gain = isMonitoringInputs() || isAudioOutNode ? object->getInputGain() : object->getGain();
            channelStrip.setVolume (Decibels::gainToDecibels (gain, -60.f));
            channelStrip.setPower (! object->isSuspended());
            b1.unblock();
            b2.unblock();
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

        stabilizeContent();
        startTimer (meterSpeedMillis);

        if (onNodeChanged)
            onNodeChanged();
    }

    inline Node getNode() const { return node; }
    
    /** @internal */
    inline void comboBoxChanged (ComboBox* box) override
    {    
        if (box == &flowBox)
        {
            updateComboBoxes (false, true);
            updateChannelStrip();
        }
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
    
    int meterSpeedMillis = 60;
    bool isAudioOutNode = false;
    bool isAudioInNode  = false;
    bool monoMeter      = false;

    SignalConnection nodeSelectedConnection;
    SignalConnection volumeChangedConnection;
    SignalConnection powerChangedConnection;
    SignalConnection volumeDoubleClickedConnection;

    inline bool isMonitoringInputs() const { return flowBox.getSelectedId() == 1; }
    inline bool isMonitoringOutputs() const { return flowBox.getSelectedId() == 1; }

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
                flowBox.setVisible (true);
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
                channelBox.setVisible (true);
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

    void volumeChanged (double value)
    {
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

    void powerChanged()
    {
        if (node.isValid())
            node.setProperty (Tags::bypass, ! channelStrip.isPowerOn());
        if (auto* obj = node.getGraphNode())
            if (auto* proc = obj->getAudioProcessor())
                proc->suspendProcessing (! channelStrip.isPowerOn());
    }

    void setUnityGain()
    {
        channelStrip.setVolume (0.0);
    }
};

}
