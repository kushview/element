
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/views/NodeChannelStripView.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/LookAndFeel.h"
#include "session/Session.h"
#include "Globals.h"
#include "Signals.h"

namespace Element {

class NodeChannelStripView::Content : public NodeChannelStripComponent
{
public:
    Content (GuiController& g)
        : NodeChannelStripComponent (g)
    {
        bindSignals();
    }

    ~Content()
    {
        unbindSignals();
    }
    
    void paint (Graphics& g) override
    {
        NodeChannelStripComponent::paint (g);
        g.setColour (LookAndFeel::contentBackgroundColor);
        g.drawLine (0.0, 0.0, 0.0, getHeight());
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
        content->setBounds (getLocalBounds());
}

void NodeChannelStripView::stabilizeContent()
{
    if (content)
        content->nodeSelected();
    disableIfNotUnlocked();
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
