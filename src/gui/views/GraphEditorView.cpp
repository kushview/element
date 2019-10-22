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

#include "gui/GuiCommon.h"
#include "gui/views/GraphEditorView.h"
#include "session/UnlockStatus.h"
#include "Common.h"

#ifndef EL_GRAPH_EDITOR_VIEWPORT
 #define EL_GRAPH_EDITOR_VIEWPORT 0
#endif

namespace Element
{

GraphEditorView::GraphEditorView()
{
    setName ("GraphEditor");
   #if EL_GRAPH_EDITOR_VIEWPORT
    addAndMakeVisible (view);
    view.setViewedComponent (&graph, false);
    view.setScrollBarsShown (true, true, false, false);
    view.setScrollOnDragEnabled (false);
    view.setBounds (graph.getLocalBounds());
   #else
    addAndMakeVisible (graph);
   #endif

    setWantsKeyboardFocus (true);
}

GraphEditorView::~GraphEditorView()
{
    nodeSelectedConnection.disconnect();
   #if EL_GRAPH_EDITOR_VIEWPORT
    view.setViewedComponent (nullptr, false);
   #endif
}

void GraphEditorView::willBeRemoved()
{
    graph.setNode (Node());
}

bool GraphEditorView::keyPressed (const KeyPress& key, Component* c)
{
    if (key.getKeyCode() == KeyPress::backspaceKey ||
        key.getKeyCode() == KeyPress::deleteKey)
    {
        graph.deleteSelectedNodes();
        return true;
    }

    return ContentView::keyPressed (key, c);
}

void GraphEditorView::stabilizeContent()
{
    if (! nodeSelectedConnection.connected())
    {
        if (auto* const cc = ViewHelpers::findContentComponent (this))
        {
            auto& gui = *cc->getAppController().findChild<GuiController>();
            nodeSelectedConnection = gui.nodeSelected.connect (
                std::bind (&GraphEditorView::onNodeSelected, this));
        }
    }

    if (! getGraph().isValid() || ! getGraph().isGraph())
    {
        if (auto session = ViewHelpers::getSession (this))
            setNode (session->getCurrentGraph());
    }
    
    const auto g = getGraph();
    
    if (g.isGraph() && !g.isRootGraph())
    {
        disableIfNotUnlocked();
    }
    else 
    {
        setEnabled (true); 
        setInterceptsMouseClicks (true, true);
    }

    graph.setNode (g);
}

void GraphEditorView::didBecomeActive()
{
    if (!getGraph().isValid() || !getGraph().isGraph())
    {
        if (auto session = ViewHelpers::getSession (this))
            setNode (session->getCurrentGraph());
    }
    else
    {
        stabilizeContent();
    }

    graph.updateComponents();
}

void GraphEditorView::paint (Graphics& g)
{
    g.fillAll (LookAndFeel::contentBackgroundColor);
}

void GraphEditorView::graphDisplayResized (const Rectangle<int> &area)
{
   #if EL_GRAPH_EDITOR_VIEWPORT
    view.setBounds (area);
    graph.setBounds (area);
   #else
    graph.setBounds (area);
   #endif
}

void GraphEditorView::graphNodeChanged (const Node& g, const Node&)
{
    stabilizeContent();
}

void GraphEditorView::onNodeSelected()
{
    if (auto* const cc = ViewHelpers::findContentComponent (this))
    {
        auto& gui = *cc->getAppController().findChild<GuiController>();
        const auto selected = gui.getSelectedNode();
        graph.selectNode (selected);
    }
}

} /* namespace Element */
