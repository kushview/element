
#include "ElementApp.h"
#include "controllers/GuiController.h"
#include "engine/GraphNode.h"
#include "gui/ChannelStripComponent.h"
#include "Signals.h"

namespace Element {

class NodeChannelStripComponent : public Component,
                                  public Timer
{
public:
    NodeChannelStripComponent (GuiController& g, bool handleNodeSelected = true)
        : gui (g), listenForNodeSelected (handleNodeSelected)
    {
        addAndMakeVisible (channelStrip);
        addAndMakeVisible (nodeName);
        nodeName.setText ("", dontSendNotification);
        nodeName.setJustificationType (Justification::centredBottom);
        nodeName.setEditable (false, true, false);
        nodeName.setFont (10.f);
        bindSignals();
    }

    ~NodeChannelStripComponent()
    {
        unbindSignals();
    }

    void bindSignals()
    {
        unbindSignals();
        if (listenForNodeSelected)
            nodeSelectedConnection = gui.nodeSelected.connect (
                std::bind (&NodeChannelStripComponent::nodeSelected, this));
        volumeChangedConnection = channelStrip.volumeChanged.connect (
            std::bind (&NodeChannelStripComponent::volumeChanged, this, std::placeholders::_1));
        powerChangedConnection = channelStrip.powerChanged.connect (
            std::bind (&NodeChannelStripComponent::powerChanged, this));
    }

    void unbindSignals()
    {
        nodeSelectedConnection.disconnect();
        volumeChangedConnection.disconnect();
        powerChangedConnection.disconnect();
    }

    void resized() override
    {
        auto r (getLocalBounds());
        nodeName.setBounds (r.removeFromTop(22).reduced (2));
        channelStrip.setBounds (r.removeFromBottom (jmin (240, r.getHeight())));
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
            if (isAudioOutNode)
                for (int c = 0; c < 2; ++c)
                    meter.setValue (c, ptr->getInputRMS (c));
            else
                for (int c = 0; c < 2; ++c)
                    meter.setValue (c, ptr->getOutpputRMS (c));
            channelStrip.setPower (! ptr->isSuspended(), false);
        }
        else
        {
            meter.resetPeaks();
            stopTimer();
        }

        meter.refresh();
        meter.repaint();
    }

    inline void setNode (const Node& newNode)
    {
        node = newNode;
        isAudioOutNode = node.isAudioOutputNode();
        
        if (node.isValid())
        {
            nodeName.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
        }

        if (GraphNodePtr object = node.getGraphNode())
        {
            shared_connection_block b1 (volumeChangedConnection);
            shared_connection_block b2 (powerChangedConnection);
            float gain = isAudioOutNode ? object->getInputGain() : object->getGain();
            channelStrip.setVolume (Decibels::gainToDecibels (gain, -60.f));
            channelStrip.setPower (! object->isSuspended());
            b1.unblock();
            b2.unblock();
        }

        startTimer (meterSpeedMillis);
    }

private:
    friend class NodeChannelStripView;
    GuiController& gui;
    Label nodeName;
    Node node;
    ChannelStripComponent channelStrip;
    bool listenForNodeSelected;
    boost::signals2::connection nodeSelectedConnection;
    boost::signals2::connection volumeChangedConnection;
    boost::signals2::connection powerChangedConnection;

    int meterSpeedMillis = 44;
    bool isAudioOutNode = false;

    void nodeSelected()
    {
        setNode (gui.getSelectedNode());
    }

    void volumeChanged (double value)
    {
        if (GraphNodePtr object = node.getGraphNode())
        {
            auto gain = Decibels::decibelsToGain (value, -60.0);
            if (node.isAudioOutputNode())
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
};

}
