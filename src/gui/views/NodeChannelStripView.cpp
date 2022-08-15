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

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "gui/views/NodeChannelStripView.h"
#include "gui/NodeChannelStripComponent.h"
#include "gui/LookAndFeel.h"
#include "session/session.hpp"
#include "context.hpp"
#include "signals.hpp"

namespace element {

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
}

void NodeChannelStripView::initializeView (AppController& app)
{
    auto& gui = *app.findChild<GuiController>();
    content.reset (new Content (gui));
    addAndMakeVisible (content.get());
    resized();
    repaint();
}

} // namespace element
