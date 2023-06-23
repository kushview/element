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

#include "ElementApp.h"

#include <element/services.hpp>
#include <element/ui.hpp>
#include "services/sessionservice.hpp"
#include "engine/graphnode.hpp"
#include "gui/nodes/AudioIONodeEditor.h"
#include "gui/nodes/AudioRouterEditor.h"
#include "gui/nodes/GenericNodeEditor.h"
#include "gui/nodes/MidiIONodeEditor.h"
#include "gui/views/NodeEditorContentView.h"
#include "gui/widgets/AudioDeviceSelectorComponent.h"
#include "gui/ViewHelpers.h"
#include <element/ui/style.hpp>
#include "gui/ContextMenus.h"
#include "gui/NodeEditorFactory.h"
#include <element/devices.hpp>
#include <element/context.hpp>

namespace element {

class NodeEditorContentView::NodeWatcher : private ValueTree::Listener
{
public:
    NodeWatcher()
    {
        data.addListener (this);
    }
    virtual ~NodeWatcher() noexcept
    {
        data.removeListener (this);
    }

    void setNodeToWatch (const Node& newNode)
    {
        if (node == newNode)
            return;
        node = newNode;
        data = node.data().getParent().getParent();
    }

    Node getWatchedNode() const { return node; }

    std::function<void()> onSiblingNodeAdded;
    std::function<void()> onSiblingNodeRemoved;
    std::function<void()> onNodesReOrdered;
    std::function<void()> onNodeNameChanged;

private:
    Node node;
    ValueTree data;

    friend class juce::ValueTree;
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override
    {
        // watched node changed
        if (tree == node.data() && property == tags::name)
        {
            if (onNodeNameChanged)
                onNodeNameChanged();
        }

        // Sibling node name changed
        if (property == tags::name && data.getChildWithName (tags::nodes).indexOf (tree) > 0)
        {
            if (onNodeNameChanged)
                onNodeNameChanged();
        }
    }

    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override
    {
        if (parent.hasType (tags::nodes) && child.hasType (tags::node) && child != data)
        {
            if (onSiblingNodeAdded)
                onSiblingNodeAdded();
        }
    }

    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override
    {
        if (parent.hasType (tags::nodes) && child.hasType (tags::node) && child != data)
        {
            if (onSiblingNodeRemoved)
                onSiblingNodeRemoved();
        }
    }

    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override
    {
        ignoreUnused (oldIndex, newIndex);
        if (parent.hasType (tags::nodes) && parent == node.data().getParent())
            if (onNodesReOrdered)
                onNodesReOrdered();
    }

    void valueTreeParentChanged (ValueTree& tree) override
    {
        ignoreUnused (tree);
    }

    void valueTreeRedirected (ValueTree&) override {}
};

NodeEditorContentView::NodeEditorContentView()
{
    setName ("NodeEditorContentView");
    addAndMakeVisible (nodesCombo);
    nodesCombo.addListener (this);

    addAndMakeVisible (menuButton);
    menuButton.setIcon (Icon (getIcons().falBarsOutline,
                              findColour (TextButton::textColourOffId)));
    menuButton.setTriggeredOnMouseDown (true);
    menuButton.onClick = [this]() {
#if 0
        NodePopupMenu menu (node, [this](NodePopupMenu& nodeMenu) {
            nodeMenu.addItem (1, "Sticky", true, isSticky());
        });
#else
        PopupMenu menu;
        menu.addItem (1, "Sticky", true, isSticky());
#endif
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&menuButton),
                            ModalCallbackFunction::forComponent (nodeMenuCallback, this));
    };

    watcher.reset (new NodeWatcher());
    watcher->onSiblingNodeAdded = watcher->onSiblingNodeRemoved = watcher->onNodesReOrdered = [this]() {
        nodesCombo.addNodes (graph, dontSendNotification);
    };

    watcher->onNodeNameChanged = [this]() {
        nodesCombo.addNodes (graph, dontSendNotification);
    };
}

NodeEditorContentView::~NodeEditorContentView()
{
    watcher.reset();
    menuButton.onClick = nullptr;
    nodesCombo.removeListener (this);
    selectedNodeConnection.disconnect();
    graphChangedConnection.disconnect();
    sessionLoadedConnection.disconnect();
}

void NodeEditorContentView::getState (String& state)
{
    ValueTree tree ("state");
    // DBG("[element] ned saving node: " << node.getUuidString());
    tree.setProperty (tags::node, node.getUuidString(), nullptr)
        .setProperty ("sticky", sticky, nullptr);

    MemoryOutputStream mo;
    {
        GZIPCompressorOutputStream gzipStream (mo, 9);
        tree.writeToStream (gzipStream);
    }

    state = mo.getMemoryBlock().toBase64Encoding();
}

void NodeEditorContentView::setState (const String& state)
{
    MemoryBlock mb;
    mb.fromBase64Encoding (state);
    const ValueTree tree = (mb.getSize() > 0)
                               ? ValueTree::readFromGZIPData (mb.getData(), mb.getSize())
                               : ValueTree();
    if (! tree.isValid())
        return;

    setSticky ((bool) tree.getProperty ("sticky", sticky));

    auto session = ViewHelpers::getSession (this);
    if (session == nullptr)
    {
        jassertfalse;
        return;
    }

    const auto nodeStr = tree[tags::node].toString();
    Node newNode;
    if (nodeStr.isNotEmpty())
    {
        const Uuid gid (nodeStr);
        newNode = session->findNodeById (gid);
    }

    if (newNode.isValid())
    {
        setNode (newNode);
    }
    else
    {
        DBG ("[element] couldn't find node to to select in node editor");
    }
}

void NodeEditorContentView::nodeMenuCallback (int result, NodeEditorContentView* view)
{
    if (result == 1)
    {
        view->setSticky (! view->isSticky());
    }
}

void NodeEditorContentView::paint (Graphics& g)
{
    g.fillAll (element::Colors::backgroundColor);
}

void NodeEditorContentView::comboBoxChanged (ComboBox*)
{
    const auto selectedNode = graph.getNode (nodesCombo.getSelectedItemIndex());
    if (selectedNode.isValid())
    {
        if (sticky)
            setNode (selectedNode);
        ViewHelpers::findContentComponent (this)->services().find<GuiService>()->selectNode (selectedNode);
    }
}

void NodeEditorContentView::resized()
{
    auto r1 = getLocalBounds().reduced (2);
    if (r1.getHeight() < 44)
        r1.setHeight (44);

    r1.removeFromTop (4);
    auto r2 = r1.removeFromTop (20);
    nodesCombo.setBounds (r2.removeFromLeft (jmax (100, r2.getWidth() - 24)));
    menuButton.setBounds (r2.withWidth (22).withX (r2.getX() + 2));

    if (editor)
    {
        r1.removeFromTop (2);
        editor->setBounds (r1);
    }
}

void NodeEditorContentView::setSticky (bool shouldBeSticky)
{
    if (sticky == shouldBeSticky)
        return;
    sticky = shouldBeSticky;
    resized();
}

void NodeEditorContentView::onGraphChanged()
{
    std::clog << "NodeEditorContentView::onGraphChanged()\n";
}

void NodeEditorContentView::onSessionLoaded()
{
    if (auto session = ViewHelpers::getSession (this))
        setNode (session->getActiveGraph().getNode (0));
}

void NodeEditorContentView::stabilizeContent()
{
    auto* cc = ViewHelpers::findContentComponent (this);
    auto session = ViewHelpers::getSession (this);
    jassert (cc && session);
    auto& gui = *cc->services().find<GuiService>();
    auto& sessions = *cc->services().find<SessionService>();

    if (! selectedNodeConnection.connected())
        selectedNodeConnection = gui.nodeSelected.connect (std::bind (
            &NodeEditorContentView::stabilizeContent, this));
    if (! sessionLoadedConnection.connected())
        sessionLoadedConnection = sessions.sessionLoaded.connect (std::bind (
            &NodeEditorContentView::onSessionLoaded, this));

    if (! sticky || ! node.isValid())
    {
        setNode (gui.getSelectedNode());
    }

    if (! node.isValid())
    {
        setNode (session->getActiveGraph().getNode (0));
    }
}

void NodeEditorContentView::setNode (const Node& newNode)
{
    auto newGraph = newNode.getParentGraph();
    bool graphChanged = false;
    if (newGraph != graph)
    {
        graphChanged = true;
        graph = newGraph;
    }

    if (graphChanged || nodesCombo.getNumItems() != graph.getNumNodes())
        nodesCombo.addNodes (graph, dontSendNotification);

    if (newNode != node)
    {
        nodeObjectValue.removeListener (this);
        clearEditor();
        watcher->setNodeToWatch (newNode);
        node = watcher->getWatchedNode();
        nodeObjectValue = node.getPropertyAsValue (tags::object, true);
        editor.reset (createEmbededEditor());
        if (editor)
            addAndMakeVisible (editor.get());

        nodeObjectValue.addListener (this);

        resized();
    }

    nodesCombo.selectNode (node, dontSendNotification);
}

void NodeEditorContentView::valueChanged (Value& value)
{
    if (! nodeObjectValue.refersToSameSourceAs (value))
        return;
    if (nodeObjectValue.getValue().getObject() == nullptr)
    {
        // node was probably removed and going to be deleted
        clearEditor();
        nodesCombo.addNodes (graph);
    }
}

void NodeEditorContentView::clearEditor()
{
    if (editor == nullptr)
        return;
    ProcessorPtr object = node.getObject();
    auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
    if (auto* aped = dynamic_cast<AudioProcessorEditor*> (editor.get()))
    {
        if (proc)
            proc->editorBeingDeleted (aped);
    }

    removeChildComponent (editor.get());
    editor.reset (nullptr);
}

Component* NodeEditorContentView::createEmbededEditor()
{
    auto* const world = ViewHelpers::getGlobals (this);
    jassert (world);
    auto& app = ViewHelpers::findContentComponent (this)->services();

    if (node.isAudioInputNode())
    {
        if (app.getRunMode() == RunMode::Standalone)
        {
            if (node.isChildOfRootGraph())
            {
                return new element::AudioDeviceSelectorComponent (world->devices(),
                                                                  1,
                                                                  DeviceManager::maxAudioChannels,
                                                                  0,
                                                                  0,
                                                                  false,
                                                                  false,
                                                                  false,
                                                                  false);
            }
            else
            {
                return nullptr;
            }
        }

        return new AudioIONodeEditor (node, world->devices(), true, false);
    }

    if (node.isAudioOutputNode())
    {
        if (app.getRunMode() == RunMode::Standalone)
        {
            if (node.isChildOfRootGraph())
            {
                return new element::AudioDeviceSelectorComponent (world->devices(),
                                                                  0,
                                                                  0,
                                                                  1,
                                                                  DeviceManager::maxAudioChannels,
                                                                  false,
                                                                  false,
                                                                  false,
                                                                  false);
            }
            else
            {
                return nullptr;
            }
        }

        return new AudioIONodeEditor (node, world->devices(), false, true);
    }

    NodeEditorFactory factory (*app.find<GuiService>());
    if (auto editor = factory.instantiate (node, NodeEditorPlacement::NavigationPanel))
        return editor.release();

    ProcessorPtr object = node.getObject();
    auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
    if (proc != nullptr && node.getFormat() == "Element" && proc->hasEditor())
        return proc->createEditor();

    return proc != nullptr ? new GenericNodeEditor (node) : nullptr;
}

} // namespace element
