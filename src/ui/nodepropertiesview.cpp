// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/services.hpp>
#include <element/ui.hpp>
#include <element/ui/style.hpp>

#include "services/sessionservice.hpp"
#include "ui/nodeproperties.hpp"
#include "ui/nodepropertiesview.hpp"
#include "ui/viewhelpers.hpp"

#define EL_PROGRAM_NAME_PLACEHOLDER "Name..."

namespace element {

namespace detail {
static inline String ioNodeMessage (const Node& node)
{
    String msg = node.getName();
    msg << ": " << TRANS ("see preferences") << "...";
    return msg;
}
} // namespace detail

class NodePropertiesView::NodeWatcher : private ValueTree::Listener
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

//==============================================================================
void NodePropertiesView::init()
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    setInterceptsMouseClicks (true, true);
    addAndMakeVisible (combo);
    combo.setFilter (NodeListComboBox::allowAllNodes);
    combo.onChange = [this]() {
        setNode (combo.selectedNode());
    };

    addAndMakeVisible (menuButton);
    menuButton.setIcon (Icon (getIcons().falBarsOutline,
                              findColour (TextButton::textColourOffId)));
    menuButton.setTriggeredOnMouseDown (true);
    menuButton.onClick = [this]() {
        PopupMenu menu;
        menu.addItem (1, "Sticky", true, isSticky());
        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&menuButton),
                            ModalCallbackFunction::forComponent (nodeMenuCallback, this));
    };

    addAndMakeVisible (props);

    watcher.reset (new NodeWatcher());
    watcher->onSiblingNodeAdded = watcher->onSiblingNodeRemoved = watcher->onNodesReOrdered = [this]() {
        combo.addNodes (_graph, dontSendNotification);
    };

    watcher->onNodeNameChanged = [this]() {
        combo.addNodes (_graph, dontSendNotification);
    };
}

NodePropertiesView::NodePropertiesView()
{
    init();
}

NodePropertiesView::NodePropertiesView (const Node& node)
{
    init();
    setNode (node);
}

NodePropertiesView::~NodePropertiesView()
{
    midiProgramChangedConnection.disconnect();
    selectedNodeConnection.disconnect();
    sessionLoadedConnection.disconnect();
}

void NodePropertiesView::paint (Graphics& g)
{
    ContentView::paint (g);
}

void NodePropertiesView::paintOverChildren (Graphics& g) {
    if (! _node.isIONode())
        return;
    g.setFont (Font (20.f));
    g.setColour (findColour (Label::textColourId));
    g.drawText (detail::ioNodeMessage(_node), 
                getLocalBounds().toFloat(), Justification::centred, true);
}

void NodePropertiesView::resized()
{
    auto r1 (getLocalBounds().reduced (2));
    r1.removeFromTop (4);
    auto r2 = r1.removeFromTop (20);
    combo.setBounds (r2.removeFromLeft (std::max (100, r2.getWidth() - 24)));
    menuButton.setBounds (r2.withWidth (22).withX (r2.getX() + 2));
    r1.removeFromTop (2);
    props.setBounds (r1);
}

void NodePropertiesView::setNode (const Node& newNode)
{
    midiProgramChangedConnection.disconnect();

    auto newGraph = newNode.getParentGraph();
    const bool graphChanged = newGraph != _graph;
    _graph = newGraph;

    if (graphChanged || combo.getNumItems() != _graph.getNumNodes())
        combo.addNodes (_graph, dontSendNotification);

    auto nextNode = newNode;
    if (! combo.nodes().contains (nextNode) && ! nextNode.isIONode())
        nextNode = combo.nodes().getFirst();

    if (nextNode != _node)
    {
        nodeSync.setNode (nextNode);
        _node = nodeSync.getNode();
        resized();
    }

    if (_node != combo.selectedNode())
        combo.selectNode (_node, dontSendNotification);

    if (ProcessorPtr ptr = _node.getObject())
    {
        midiProgramChangedConnection = ptr->midiProgramChanged.connect (
            std::bind (&NodePropertiesView::updateMidiProgram, this));
    }

    updateProperties();
}

void NodePropertiesView::setSticky (bool shouldBeSticky)
{
    if (sticky == shouldBeSticky)
        return;
    sticky = shouldBeSticky;
    resized();
}

void NodePropertiesView::stabilizeContent()
{
    auto* cc = ViewHelpers::findContentComponent (this);
    auto session = ViewHelpers::getSession (this);

    if (cc == nullptr || session == nullptr)
    {
        jassertfalse;
        return;
    }

    auto& gui = *cc->services().find<GuiService>();
    if (! selectedNodeConnection.connected())
        selectedNodeConnection = gui.nodeSelected.connect (std::bind (
            &NodePropertiesView::stabilizeContent, this));

    auto& sessions = *cc->services().find<SessionService>();
    if (! sessionLoadedConnection.connected())
        sessionLoadedConnection = sessions.sigSessionLoaded.connect (std::bind (
            &NodePropertiesView::onSessionLoaded, this));

    if (! sticky || ! _node.isValid())
    {
        setNode (gui.getSelectedNode());
    }

    if (! _node.isValid())
    {
        setNode (session->getActiveGraph().getNode (0));
    }
}

//==============================================================================
void NodePropertiesView::getState (String& state)
{
    ValueTree tree ("state");
    tree.setProperty (tags::node, _node.getUuidString(), nullptr)
        .setProperty ("sticky", sticky, nullptr);

    MemoryOutputStream mo;
    {
        GZIPCompressorOutputStream gzipStream (mo, 9);
        tree.writeToStream (gzipStream);
    }

    state = mo.getMemoryBlock().toBase64Encoding();
}

void NodePropertiesView::setState (const String& state)
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
        DBG ("[element] couldn't find node to to select in node properties view");
    }
}

//==============================================================================
void NodePropertiesView::onSessionLoaded()
{
    if (auto session = ViewHelpers::getSession (this))
        setNode (session->getActiveGraph().getNode (0));
}

void NodePropertiesView::nodeMenuCallback (int result, NodePropertiesView* view)
{
    if (result == 1)
    {
        view->setSticky (! view->isSticky());
    }
}

//==============================================================================
void NodePropertiesView::updateProperties()
{
    props.clear();
    if (_node.isValid())
    {
        if (_node.isIONode())
        {
            props.setMessageWhenEmpty ("");
        }
        else
        {
            String nodeName = "Node";
            if (_node.getPluginName().isNotEmpty())
                nodeName << " - " << _node.getPluginName();
            props.addSection (nodeName, NodeProperties (_node, true, false));
            props.addSection ("MIDI", NodeProperties (_node, false, true));
            props.setMessageWhenEmpty ("");
        }
    }
    resized();
    repaint();
}

void NodePropertiesView::updateMidiProgram()
{
    updateProperties();
}

} // namespace element
