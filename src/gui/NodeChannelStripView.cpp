
#include "gui/NodeChannelStripView.h"
#include "gui/ChannelStripComponent.h"
#include "gui/LookAndFeel.h"

namespace Element {

class NodeChannelStripView::Content : public Component 
{
public:
    Content()
    {
        addAndMakeVisible (channelStrip);
    }

    void resized() override
    {
        channelStrip.setBounds (getLocalBounds());
    }

    void paint (Graphics& g) override
    {
        g.setColour (LookAndFeel::widgetBackgroundColor);
        g.fillAll();
    }

private:
    ChannelStripComponent channelStrip;
};

NodeChannelStripView::NodeChannelStripView()
{
    content.reset (new Content());
    addAndMakeVisible (content.get());
}

NodeChannelStripView::~NodeChannelStripView()
{
    content = nullptr;
}

void NodeChannelStripView::resized()
{
    content->setBounds (getLocalBounds().reduced (2));
}

void NodeChannelStripView::stabilizeContent()
{

}

void NodeChannelStripView::initializeView (AppController&)
{

}

}
