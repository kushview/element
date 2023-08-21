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

#include <element/services.hpp>
#include <element/ui.hpp>
#include <element/ui/style.hpp>
#include <element/session.hpp>
#include <element/context.hpp>
#include <element/signals.hpp>

#include "ui/nodechannelstripview.hpp"
#include "ui/nodechannelstrip.hpp"

namespace element {

class NodeChannelStripView::Content : public NodeChannelStripComponent
{
public:
    Content (GuiService& g)
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
        g.setColour (Colors::contentBackgroundColor);
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

void NodeChannelStripView::initializeView (Services& app)
{
    auto& gui = *app.find<GuiService>();
    content.reset (new Content (gui));
    addAndMakeVisible (content.get());
    resized();
    repaint();
}

} // namespace element
