// Copyright 2019-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/guicommon.hpp"
#include "ui/block.hpp"
#include "ui/grapheditorview.hpp"
#include "common.hpp"
#include "scopedcallback.hpp"
#include "engine/midiengine.hpp"

namespace element {

GraphEditorView::GraphEditorView()
{
    init();
}

GraphEditorView::GraphEditorView (const Node& g)
    : node (g)
{
    init();
}

void GraphEditorView::init()
{
    setName (EL_VIEW_GRAPH_EDITOR);
    addAndMakeVisible (_editor);
    setSize (640, 360);
    setWantsKeyboardFocus (true);
}

GraphEditorView::~GraphEditorView()
{
    nodeSelectedConnection.disconnect();
    nodeRemovedConnection.disconnect();
    sessionLoadedConnection.disconnect();
}

void GraphEditorView::willBeRemoved()
{
    auto* world = ViewHelpers::getGlobals (this);
    jassert (world); // something went majorly wrong...
    if (world)
        world->midi().removeChangeListener (this);
    saveSettings();
    _editor.setNode (Node());
}

bool GraphEditorView::keyPressed (const KeyPress& key)
{
    if (key.getKeyCode() == KeyPress::backspaceKey || key.getKeyCode() == KeyPress::deleteKey)
    {
        _editor.deleteSelectedNodes();
        return true;
    }

    return ContentView::keyPressed (key);
}

void GraphEditorView::parentHierarchyChanged()
{
    if (node.isValid())
    {
        setNode (node);
        node = Node();
        stabilizeContent();
    }
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
            auto& eng = *gui.sibling<EngineService>();
            nodeRemovedConnection = eng.sigNodeRemoved.connect (
                std::bind (&GraphEditorView::onNodeRemoved, this, std::placeholders::_1));
            auto& s = *cc->services().find<SessionService>();
            sessionLoadedConnection = s.sigSessionLoaded.connect (
                std::bind (&GraphEditorView::onSessionLoaded, this));
        }
    }

    if (! getGraph().isValid() || ! getGraph().isGraph())
    {
        if (auto session = ViewHelpers::getSession (this))
            setNode (session->getCurrentGraph());
    }

    const auto g = getGraph();
    _editor.setNode (g);
    onNodeSelected();
}

void GraphEditorView::didBecomeActive()
{
    auto* world = ViewHelpers::getGlobals (this);
    jassert (world); // something went majorly wrong...
    world->midi().addChangeListener (this);
    stabilizeContent();
    restoreSettings();
    _editor.updateComponents();
}

void GraphEditorView::changeListenerCallback (ChangeBroadcaster*)
{
    _editor.stabilizeNodes();
}

void GraphEditorView::paint (Graphics& g)
{
    g.fillAll (Colors::contentBackgroundColor);
}

void GraphEditorView::graphDisplayResized (const Rectangle<int>& area)
{
    auto r = area;
    _editor.setBounds (r);

    auto s = settings();
    if (s.isValid())
    {
        s.setProperty (tags::width, _editor.getWidth(), nullptr)
            .setProperty (tags::height, _editor.getHeight(), nullptr);
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

void GraphEditorView::onSessionLoaded()
{
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
            _editor.selectNode (selected);
        }
    }
}

void GraphEditorView::onNodeRemoved (const Node& rnode)
{
    if (rnode.isGraph() && rnode == getGraph())
    {
        auto nextGraph = Node();
        if (auto session = ViewHelpers::getSession (this))
            nextGraph = session->getActiveGraph();
        setNode (nextGraph);
    }
}

void GraphEditorView::updateSizeInternal (const bool force)
{
#if 0
    auto r = _editor.getRequiredSpace();
    const auto rc = r;
    if (r.getWidth() <= view.getWidth())
        r.setWidth (view.getWidth());
    if (r.getHeight() <= view.getHeight())
        r.setHeight (view.getHeight());
    if (force || r != rc)
        _editor.setBounds (r);
#endif
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

#if 0
    _editor.setSize (s.getProperty (tags::width, getWidth()),
                     s.getProperty (tags::height, getHeight()));
   
    _editor.setZoomScale (s.getProperty ("zoomScale", 1.0f));
#endif

    resized();
}

void GraphEditorView::saveSettings()
{
    auto s = settings();
    if (! s.isValid())
        return;

    s.setProperty (tags::width, _editor.getWidth(), nullptr);
    s.setProperty (tags::height, _editor.getHeight(), nullptr);
    // s.setProperty ("horizontalRangeStart", view.getHorizontalScrollBar().getCurrentRangeStart(), nullptr);
    // s.setProperty ("verticalRangeStart", view.getVerticalScrollBar().getCurrentRangeStart(), nullptr);
    // s.setProperty ("zoomScale", _editor.getZoomScale(), nullptr);
}

void GraphEditorView::selectAllNodes() { _editor.selectAllNodes(); }

} /* namespace element */
