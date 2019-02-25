
#include "gui/views/SessionTreeContentView.h"
#include "gui/SessionTreePanel.h"
#include "Common.h"

namespace Element {

SessionTreeContentView::SessionTreeContentView()
{
    tree.reset (new SessionTreePanel());
    addAndMakeVisible (tree.get());
}

SessionTreeContentView::~SessionTreeContentView()
{
    tree.reset();
}

void SessionTreeContentView::didBecomeActive()
{

}

void SessionTreeContentView::stabilizeContent()
{
    if (auto session = ViewHelpers::getSession (this))
        tree->setSession (session);
    resized();
    repaint();
}

void SessionTreeContentView::resized()
{
    tree->setBounds (getLocalBounds());
}

}
