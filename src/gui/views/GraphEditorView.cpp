// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "gui/GuiCommon.h"
#include "ui/block.hpp"
#include "gui/views/GraphEditorView.h"
#include "common.hpp"
#include "scopedcallback.hpp"
#include "engine/midiengine.hpp"

namespace element {

GraphEditorView::GraphEditorView()
{
    setName (EL_VIEW_GRAPH_EDITOR);

    graph.onBlockMoved = [this] (BlockComponent& block) {
        const int resizeBy = 12;
        const int edgeSpeed = 6;
        const int maxSpeed = 10;

        // restrict top/left out of bounds scroll
        auto pos = block.getBounds().getTopLeft();
        if (block.getX() < 0)
            pos.setX (0);
        if (block.getY() < 0)
            pos.setY (0);

        // save top left
        const auto revertTopLeftPos = pos;
        const bool revertTopLeft = pos.getX() != block.getX() || pos.getY() != block.getY();
        ScopedCallback defer ([&block, &revertTopLeftPos, &revertTopLeft]() {
            if (revertTopLeft)
            {
                block.setNodePosition (revertTopLeftPos);
                block.updatePosition();
            }
        });

        // no action if mouse within viewable area
        const auto mp = view.getLocalPoint (nullptr, Desktop::getInstance().getMousePosition());
        if (mp.getX() > 0 && mp.getX() < view.getViewWidth() && mp.getY() > 0 && mp.getY() < view.getViewHeight())
        {
            return;
        }

        // expand and scroll bottom/right
        pos = block.getBounds().getBottomRight();
        auto gb = graph.getBounds();
        bool sizeShouldChange = false;
        if (pos.x > gb.getWidth())
        {
            gb.setWidth (pos.x + resizeBy);
            sizeShouldChange = true;
        }
        if (pos.y > gb.getHeight())
        {
            gb.setHeight (pos.y + resizeBy);
            sizeShouldChange = true;
        }
        if (sizeShouldChange)
        {
            graph.setBounds (gb);
        }

        pos = view.getLocalPoint (&graph, pos.toFloat()).toInt();
        view.autoScroll (pos.x, pos.y, edgeSpeed, maxSpeed);
    };

    graph.onZoomChanged = [this]() {
        auto s = settings();
        if (s.isValid())
        {
            s.setProperty ("zoomScale", graph.getZoomScale(), nullptr);
        }
    };

    addAndMakeVisible (view);
    view.setViewedComponent (&graph, false);
    view.setScrollBarsShown (true, true, false, false);
    view.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
    view.setBounds (graph.getLocalBounds());

    addAndMakeVisible (nodeProps);
    setSize (640, 360);

    addAndMakeVisible (nodePropsToggle);
    nodePropsToggle.setText ("<<", dontSendNotification);
    nodePropsToggle.setJustificationType (Justification::centred);
    nodePropsToggle.onClick = [this]() {
        nodeProps.setVisible (! nodeProps.isVisible());
        auto s = settings();
        if (s.isValid())
            s.setProperty ("nodePropsVisible", nodeProps.isVisible(), nullptr);
        resized();
        stabilizeContent();
    };

    setWantsKeyboardFocus (true);
}

GraphEditorView::~GraphEditorView()
{
    graph.onZoomChanged = nullptr;
    graph.onBlockMoved = nullptr;
    nodeSelectedConnection.disconnect();
    nodeRemovedConnection.disconnect();
    view.setViewedComponent (nullptr, false);
}

void GraphEditorView::willBeRemoved()
{
    auto* world = ViewHelpers::getGlobals (this);
    jassert (world); // something went majorly wrong...
    if (world)
        world->midi().removeChangeListener (this);
    saveSettings();
    graph.setNode (Node());
}

bool GraphEditorView::keyPressed (const KeyPress& key)
{
    if (key.getKeyCode() == KeyPress::backspaceKey || key.getKeyCode() == KeyPress::deleteKey)
    {
        graph.deleteSelectedNodes();
        return true;
    }

    return ContentView::keyPressed (key);
}

void GraphEditorView::stabilizeContent()
{
    if (! (nodeSelectedConnection.connected() && nodeRemovedConnection.connected()))
    {
        if (auto* const cc = ViewHelpers::findContentComponent (this))
        {
            auto& gui = *cc->services().find<GuiService>();
            nodeSelectedConnection = gui.nodeSelected.connect (
                std::bind (&GraphEditorView::onNodeSelected, this));
            auto& eng = *cc->services().find<EngineService>();
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
    onNodeSelected();

    nodePropsToggle.setText (nodeProps.isVisible() ? ">>" : "<<", dontSendNotification);
}

void GraphEditorView::didBecomeActive()
{
    auto* world = ViewHelpers::getGlobals (this);
    jassert (world); // something went majorly wrong...
    world->midi().addChangeListener (this);
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
    g.fillAll (Colors::contentBackgroundColor);
}

void GraphEditorView::graphDisplayResized (const Rectangle<int>& area)
{
    auto r = area;
    if (nodeProps.isVisible())
    {
        const auto hidePropsWidth = nodePropsWidth * 1.75;
        if (getWidth() >= hidePropsWidth && nodePropsWidth > 0)
        {
            nodeProps.setBounds (r.removeFromRight (nodePropsWidth));
            r.removeFromRight (2);
        }
        else
        {
            // place offscreen
            nodeProps.setBounds (getWidth() + 2, 0, nodePropsWidth, getHeight());
        }
    }

    nodePropsToggle.setBounds (r.getRight() - 38, r.getY() + 4, 30, 18);

    view.setBounds (r);
    if (graph.getWidth() < view.getWidth() || graph.getHeight() < view.getHeight())
        graph.setBounds (view.getBounds());

    auto s = settings();
    if (s.isValid())
    {
        s.setProperty (tags::width, graph.getWidth(), nullptr)
            .setProperty (tags::height, graph.getHeight(), nullptr);
    }
}

void GraphEditorView::graphNodeWillChange()
{
    saveSettings();
}

void GraphEditorView::graphNodeChanged (const Node& g, const Node&)
{
    stabilizeContent();
    restoreSettings();
    updateSizeInternal (false);
}

void GraphEditorView::onNodeSelected()
{
    if (auto* const cc = ViewHelpers::findContentComponent (this))
    {
        auto session = cc->session();
        auto& gui = *cc->services().find<GuiService>();
        const auto selected = gui.getSelectedNode();
        if (selected.descendsFrom (getGraph()))
        {
            // prevent minor gui changes from marking session as dirty.
            // This is a hack and need a better solution;
            Session::ScopedFrozenLock freeze (*session);
            graph.selectNode (selected);
            nodeProps.addProperties (selected);
            if (nodeProps.isVisible())
                resized();
        }
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

void GraphEditorView::updateSizeInternal (const bool force)
{
    auto r = graph.getRequiredSpace();
    const auto rc = r;
    if (r.getWidth() <= view.getWidth())
        r.setWidth (view.getWidth());
    if (r.getHeight() <= view.getHeight())
        r.setHeight (view.getHeight());
    if (force || r != rc)
        graph.setBounds (r);
}

ValueTree GraphEditorView::settings() const
{
    ValueTree uivt = getGraph().getUIValueTree();
    return uivt.isValid() ? uivt.getOrCreateChildWithName ("GraphEditorView", nullptr)
                          : ValueTree();
}

void GraphEditorView::restoreSettings()
{
    auto s = settings();
    if (! s.isValid())
    {
        updateSizeInternal();
        return;
    }

    graph.setSize (s.getProperty (tags::width, getWidth()),
                   s.getProperty (tags::height, getHeight()));
    graph.setZoomScale (s.getProperty ("zoomScale", 1.0f));
    view.getHorizontalScrollBar().setCurrentRangeStart (s.getProperty ("horizontalRangeStart", 0.0));
    view.getVerticalScrollBar().setCurrentRangeStart (s.getProperty ("verticalRangeStart", 0.0));
    nodeProps.setVisible (s.getProperty ("nodePropsVisible", false));

    resized();
}

void GraphEditorView::saveSettings()
{
    auto s = settings();
    if (! s.isValid())
        return;

    s.setProperty (tags::width, graph.getWidth(), nullptr);
    s.setProperty (tags::height, graph.getHeight(), nullptr);
    s.setProperty ("horizontalRangeStart", view.getHorizontalScrollBar().getCurrentRangeStart(), nullptr);
    s.setProperty ("verticalRangeStart", view.getVerticalScrollBar().getCurrentRangeStart(), nullptr);
    s.setProperty ("zoomScale", graph.getZoomScale(), nullptr);
    s.setProperty ("nodePropsVisible", nodeProps.isVisible(), nullptr);
}

void GraphEditorView::selectAllNodes() { graph.selectAllNodes(); }

} /* namespace element */
