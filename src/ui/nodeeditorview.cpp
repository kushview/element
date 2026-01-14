// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/context.hpp>
#include <element/devices.hpp>
#include <element/services.hpp>
#include <element/ui/style.hpp>
#include <element/ui.hpp>

#include "ElementApp.h"

#include "engine/graphnode.hpp"
#include "nodes/ionodeeditor.hpp"
#include "nodes/audioroutereditor.hpp"
#include "nodes/genericeditor.hpp"
#include "nodes/midideviceeditor.hpp"

#include "services/sessionservice.hpp"
#include "ui/audiodeviceselector.hpp"
#include "ui/viewhelpers.hpp"
#include "ui/contextmenus.hpp"
#include "ui/nodeeditorfactory.hpp"
#include "ui/nodeeditorview.hpp"

namespace element {

class NodeEditorView::NodeWatcher : private ValueTree::Listener
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
        if (parent.hasType (tags::nodes) && child.hasType (types::Node) && child != data)
        {
            if (onSiblingNodeAdded)
                onSiblingNodeAdded();
        }
    }

    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override
    {
        if (parent.hasType (tags::nodes) && child.hasType (types::Node) && child != data)
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

NodeEditorView::NodeEditorView()
{
    setName ("NodeEditorView");
    addAndMakeVisible (nodesCombo);
    nodesCombo.setFilter (NodeListComboBox::rejectIONodes);
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

NodeEditorView::~NodeEditorView()
{
    watcher.reset();
    menuButton.onClick = nullptr;
    nodesCombo.removeListener (this);
    selectedNodeConnection.disconnect();
    graphChangedConnection.disconnect();
    sessionLoadedConnection.disconnect();
}

void NodeEditorView::getState (String& state)
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

void NodeEditorView::setState (const String& state)
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

void NodeEditorView::nodeMenuCallback (int result, NodeEditorView* view)
{
    if (result == 1)
    {
        view->setSticky (! view->isSticky());
    }
}

void NodeEditorView::paint (Graphics& g)
{
    g.fillAll (element::Colors::backgroundColor);
}

void NodeEditorView::comboBoxChanged (ComboBox*)
{
    const auto selectedNode = nodesCombo.selectedNode();
    if (selectedNode.isValid())
    {
        if (sticky)
            setNode (selectedNode);
        ViewHelpers::findContentComponent (this)->services().find<GuiService>()->selectNode (selectedNode);
    }
}

void NodeEditorView::resized()
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

void NodeEditorView::setSticky (bool shouldBeSticky)
{
    if (sticky == shouldBeSticky)
        return;
    sticky = shouldBeSticky;
    resized();
}

void NodeEditorView::onGraphChanged()
{
    // noop
}

void NodeEditorView::onSessionLoaded()
{
    if (auto session = ViewHelpers::getSession (this))
        setNode (session->getActiveGraph().getNode (0));
}

void NodeEditorView::stabilizeContent()
{
    auto* cc = ViewHelpers::findContentComponent (this);
    auto session = ViewHelpers::getSession (this);
    jassert (cc && session);
    auto& gui = *cc->services().find<GuiService>();
    auto& sessions = *cc->services().find<SessionService>();

    if (! selectedNodeConnection.connected())
        selectedNodeConnection = gui.nodeSelected.connect (std::bind (
            &NodeEditorView::stabilizeContent, this));
    if (! sessionLoadedConnection.connected())
        sessionLoadedConnection = sessions.sigSessionLoaded.connect (std::bind (
            &NodeEditorView::onSessionLoaded, this));

    if (! sticky || ! node.isValid())
    {
        setNode (gui.getSelectedNode());
    }

    if (! node.isValid())
    {
        setNode (session->getActiveGraph().getNode (0));
    }
}

void NodeEditorView::setNode (const Node& newNode)
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

    auto nextNode = newNode;
    if (! nodesCombo.nodes().contains (nextNode) || nextNode.isIONode())
        nextNode = nodesCombo.nodes().getFirst();

    if (nextNode != node)
    {
        nodeObjectValue.removeListener (this);
        clearEditor();
        watcher->setNodeToWatch (nextNode);
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

void NodeEditorView::valueChanged (Value& value)
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

void NodeEditorView::clearEditor()
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

Component* NodeEditorView::createEmbededEditor()
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
