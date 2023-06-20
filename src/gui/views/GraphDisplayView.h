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

#pragma once

#include "gui/GuiCommon.h"
#include <element/ui/content.hpp>
#include "gui/widgets/BreadCrumbComponent.h"
#include "session/commandmanager.hpp"

namespace element {

/** This is a simple container which displays a breadcrumb above a content area */
class GraphDisplayView : public ContentView,
                         public Button::Listener
{
public:
    GraphDisplayView()
    {
        addAndMakeVisible (breadcrumb);
        addAndMakeVisible (configButton);
        configButton.setTooltip ("Show graph settings");
        configButton.addListener (this);
        configButton.setVisible (false);

        addAndMakeVisible (sessionConfigButton);
        sessionConfigButton.setTooltip ("Show session settings");
        sessionConfigButton.addListener (this);
        sessionConfigButton.setVisible (false);
    }

    virtual ~GraphDisplayView()
    {
        configButton.removeListener (this);
    }

    inline void buttonClicked (Button* b) override
    {
        auto* const world = ViewHelpers::getGlobals (this);
        if (! world)
            return;
        if (b == &configButton)
        {
            world->getCommandManager().invokeDirectly (Commands::showGraphConfig, true);
        }
        else if (b == &sessionConfigButton)
        {
            world->getCommandManager().invokeDirectly (Commands::showSessionConfig, true);
        }
    }

    inline void setBreadCrumbVisible (const bool isVisible)
    {
        if (isVisible != breadcrumb.isVisible())
        {
            breadcrumb.setVisible (isVisible);
            resized();
        }
    }

    inline void setConfigButtonVisible (const bool isVisible)
    {
        if (isVisible != configButton.isVisible())
        {
            configButton.setVisible (isVisible);
            resized();
        }
    }

    inline void setNode (const Node& n)
    {
        Node newGraph = n.isGraph() ? n : n.getParentGraph();
        Node newNode = n.isGraph() ? Node() : n;

        if (newGraph != graph || newNode != node)
        {
            graphNodeWillChange();

            graph = newGraph;
            node = newNode;

            if (node.isValid())
                breadcrumb.setNode (node);
            else if (graph.isValid())
                breadcrumb.setNode (graph);
            else
                breadcrumb.setNode (Node());

            graphNodeChanged (graph, node);
        }
    }

    inline void resized() override
    {
        auto r = getLocalBounds();
        if (breadcrumb.isVisible())
            breadcrumb.setBounds (r.removeFromTop (24));
        graphDisplayResized (r);

        const int configButtonSize = 14;
        r = getLocalBounds().reduced (4);
        r = r.removeFromTop (configButtonSize);

        if (sessionConfigButton.isVisible())
        {
            sessionConfigButton.setBounds (r.removeFromRight (configButtonSize));
            r.removeFromRight (2);
        }

        configButton.setBounds (r.removeFromRight (configButtonSize));
    }

    Node getGraph() const { return graph; }
    Node getNode() const { return node; }

protected:
    virtual void graphDisplayResized (const Rectangle<int>& area) = 0;
    virtual void graphNodeWillChange() {}
    virtual void graphNodeChanged (const Node&, const Node&) {}

private:
    Node graph, node;
    BreadCrumbComponent breadcrumb;
    ConfigButton configButton, sessionConfigButton;
};

} // namespace element
