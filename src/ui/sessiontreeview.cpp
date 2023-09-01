// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "ui/sessiontreeview.hpp"
#include "ui/sessiontreepanel.hpp"
#include "ui/viewhelpers.hpp"
#include "common.hpp"

namespace element {

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

} // namespace element
