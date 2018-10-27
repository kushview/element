
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/NodeChannelStripView.h"
#include "gui/ChannelStripComponent.h"
#include "gui/LookAndFeel.h"
#include "session/Session.h"
#include "Globals.h"
#include "Signals.h"

namespace Element {

class NodeChannelStripView::Content : public Component,
                                      public Timer
{
public:
    Content (GuiController& g)
        : gui (g)
    {
        addAndMakeVisible (channelStrip);
        addAndMakeVisible (nodeName);
        nodeName.setText ("Node 1", dontSendNotification);
        nodeName.setJustificationType (Justification::centredBottom);
        nodeName.setEditable (false, true, false);

        graphAddedConnection = gui.nodeSelected.connect (
            std::bind (&Content::nodeSelected, this));
        volumeChangedConnection = channelStrip.volumeChanged.connect (
            std::bind (&Content::volumeChanged, this, std::placeholders::_1));
        powerChangedConnection = channelStrip.powerChanged.connect (
            std::bind (&Content::powerChanged, this));
    }

    ~Content()
    {
        graphAddedConnection.disconnect();
        volumeChangedConnection.disconnect();
    }

    void resized() override
    {
        auto r (getLocalBounds());
        nodeName.setBounds (r.removeFromTop(22).reduced (2));
        channelStrip.setBounds (r.removeFromBottom (jmin (240, r.getHeight())));
    }

    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel::widgetBackgroundColor);
        g.fillAll();
    }

    void timerCallback() override
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

private:
    friend class NodeChannelStripView;
    GuiController& gui;
    Label nodeName;
    Node node;
    ChannelStripComponent channelStrip;
    boost::signals2::connection graphAddedConnection;
    boost::signals2::connection volumeChangedConnection;
    boost::signals2::connection powerChangedConnection;

    int meterSpeedMillis = 44;
    bool isAudioOutNode = false;

    void nodeSelected()
    {
        node = gui.getSelectedNode();
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

NodeChannelStripView::NodeChannelStripView()
{
}

NodeChannelStripView::~NodeChannelStripView()
{
    content = nullptr;
}

void NodeChannelStripView::resized()
{
    if (content)
        content->setBounds (getLocalBounds().reduced (2));
}

void NodeChannelStripView::stabilizeContent()
{
    if (content)
        content->nodeSelected();
}

void NodeChannelStripView::initializeView (AppController& app)
{
    auto& gui = *app.findChild<GuiController>();
    content.reset (new Content (gui));
    addAndMakeVisible (content.get());
    resized();
    repaint();
}

}
