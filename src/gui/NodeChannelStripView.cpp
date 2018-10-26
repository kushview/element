
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
    }

    ~Content()
    {
        graphAddedConnection.disconnect();
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
    GuiController& gui;
    Label nodeName;
    Node node;
    ChannelStripComponent channelStrip;
    boost::signals2::connection graphAddedConnection;
    int meterSpeedMillis = 44;
    bool isAudioOutNode = false;
    void nodeSelected()
    {
        node = gui.getSelectedNode();
        isAudioOutNode = node.isAudioOutputNode();
        nodeName.getTextValue().referTo (node.getPropertyAsValue (Tags::name));
        startTimer (meterSpeedMillis);
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
