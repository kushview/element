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
#include "gui/BlockComponent.h"
#include "gui/views/GraphEditorView.h"
#include "Common.h"

namespace Element {

GraphEditorView::GraphEditorView()
{
    setName ("GraphEditor");

    graph.onBlockMoved = [this](BlockComponent& block) {
        if (block.getRight() <= graph.getWidth() && block.getBottom() <= graph.getHeight())
            return;
        updateSizeInternal();
    };

    addAndMakeVisible (view);
    view.setViewedComponent (&graph, false);
    view.setScrollBarsShown (true, true, false, false);
    view.setScrollOnDragEnabled (false);
    view.setBounds (graph.getLocalBounds());

    setWantsKeyboardFocus (true);
}

GraphEditorView::~GraphEditorView()
{
    nodeSelectedConnection.disconnect();
    nodeRemovedConnection.disconnect();
    view.setViewedComponent (nullptr, false);
}

void GraphEditorView::willBeRemoved()
{
    auto* world = ViewHelpers::getGlobals (this);
    jassert (world); // something went majorly wrong...
    if (world)
        world->getMidiEngine().removeChangeListener (this);
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
    if (! nodeSelectedConnection.connected() ||
        ! nodeRemovedConnection.connected())
    {
        if (auto* const cc = ViewHelpers::findContentComponent (this))
        {
            auto& gui = *cc->getAppController().findChild<GuiController>();
            nodeSelectedConnection = gui.nodeSelected.connect (
                std::bind (&GraphEditorView::onNodeSelected, this));
            auto& eng = *cc->getAppController().findChild<EngineController>();
            nodeRemovedConnection = eng.nodeRemoved.connect (
                std::bind (&GraphEditorView::onNodeRemoved, this, std::placeholders::_1));
        }
    }

    if (! getGraph().isValid() || ! getGraph().isGraph())
    {
        if (auto session = ViewHelpers::getSession (this))
            setNode (session->getCurrentGraph());
    }
    
    const auto g = getGraph();
    graph.setNode (g);
}

void GraphEditorView::didBecomeActive()
{
    auto session = ViewHelpers::getSession (this);
    auto* world = ViewHelpers::getGlobals (this);
    jassert (world); // something went majorly wrong...

    world->getMidiEngine().addChangeListener (this);
    stabilizeContent();

    restoreSettings();
    graph.updateComponents();
}

void GraphEditorView::changeListenerCallback (ChangeBroadcaster*)
{
    graph.stabilizeNodes();
}

void GraphEditorView::paint (Graphics& g)
{
    g.fillAll (LookAndFeel::contentBackgroundColor);
}

void GraphEditorView::graphDisplayResized (const Rectangle<int> &area)
{
    view.setBounds (area);
    if (graph.getWidth() < view.getWidth() || graph.getHeight() < view.getHeight())
        graph.setBounds (view.getBounds());

    auto s = getSettings();
    if (s.isValid())
    {
        s.setProperty (Tags::width,  graph.getWidth(),  nullptr)
         .setProperty (Tags::height, graph.getHeight(), nullptr); 
    }
}

void GraphEditorView::graphNodeChanged (const Node& g, const Node&)
{
    stabilizeContent();
    updateSizeInternal();
}

void GraphEditorView::onNodeSelected()
{
    if (auto* const cc = ViewHelpers::findContentComponent (this))
    {
        auto& gui = *cc->getAppController().findChild<GuiController>();
        const auto selected = gui.getSelectedNode();
        if (selected.descendsFrom (getGraph()))
            graph.selectNode (selected);
    }
}

void GraphEditorView::onNodeRemoved (const Node& node)
{
    if (node.isGraph() && node == getGraph())
    {
        auto nextGraph = Node();
        if (auto session = ViewHelpers::getSession (this))
            nextGraph = session->getActiveGraph();
        setNode (nextGraph);
    }
}

void GraphEditorView::updateSizeInternal()
{
    auto r = graph.getRequiredSpace();
    if (r.getWidth() <= view.getWidth())
        r.setWidth (view.getWidth());
    if (r.getHeight() <= view.getHeight())
        r.setHeight (view.getHeight());
    graph.setBounds (r);
}

ValueTree GraphEditorView::getSettings() const
{
    ValueTree uivt = getNode().getUIValueTree();
    return uivt.isValid() ? uivt.getOrCreateChildWithName ("GraphEditorView", nullptr)
                          : ValueTree();
}
 
void GraphEditorView::restoreSettings()
{
    auto s = getSettings();
    if (! s.isValid())
        return;
    setSize (s.getProperty (Tags::width,  getWidth()),
             s.getProperty (Tags::height, getHeight()));
}

void GraphEditorView::saveSettings()
{
    auto s = getSettings();
    if (! s.isValid())
        return;
    s.setProperty (Tags::width,  getWidth(),  nullptr);
    s.setProperty (Tags::height, getHeight(), nullptr);
}

} /* namespace Element */
