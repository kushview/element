
#include "ElementApp.h"

#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
#include "controllers/SessionController.h"

#include "gui/nodes/AudioIONodeEditor.h"
#include "gui/nodes/AudioRouterEditor.h"
#include "gui/nodes/GenericNodeEditor.h"
#include "gui/nodes/MidiIONodeEditor.h"
#include "gui/views/NodeEditorContentView.h"
#include "gui/widgets/AudioDeviceSelectorComponent.h"
#include "gui/ViewHelpers.h"
#include "gui/LookAndFeel.h"
#include "gui/ContextMenus.h"

#include "session/DeviceManager.h"
#include "Globals.h"

namespace Element {

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
        data = node.getValueTree().getParent().getParent();
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
        if (tree == node.getValueTree() && property == Tags::name) {
            if (onNodeNameChanged)
                onNodeNameChanged();
        }
    }

    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override 
    {
        if (parent.hasType (Tags::nodes) && child.hasType (Tags::node) && child != data)
        {
            if (onSiblingNodeAdded)
                onSiblingNodeAdded();
        }
    }

    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override 
    {
        if (parent.hasType (Tags::nodes) && child.hasType (Tags::node) && child != data)
        {
            if (onSiblingNodeRemoved)
                onSiblingNodeRemoved();
        }
    }

    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override
    {
        ignoreUnused (oldIndex, newIndex);
        if (parent.hasType (Tags::nodes) && parent == node.getValueTree().getParent())
            if (onNodesReOrdered)
                onNodesReOrdered();
    }

    void valueTreeParentChanged (ValueTree& tree) override
    {
        ignoreUnused (tree);
    }

    void valueTreeRedirected (ValueTree&) override { }
};

static String noteValueToString (double value)
{
    return MidiMessage::getMidiNoteName (roundToInt (value), true, true, 3);
}

NodeEditorContentView::NodeEditorContentView()
{
    setName ("NodeEditorContentView");
    addAndMakeVisible (nodesCombo);
    nodesCombo.addListener (this);

    addAndMakeVisible (menuButton);
    menuButton.setIcon (Icon (getIcons().falBarsOutline, 
        findColour (TextButton::textColourOffId)));
    menuButton.setTriggeredOnMouseDown (true);
    menuButton.onClick = [this]()
    {
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

    watcher->onNodeNameChanged = [this]()
    {
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
    DBG("[EL] ned saving node: " << node.getUuidString());
    tree.setProperty (Tags::node, node.getUuidString(), nullptr)
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

    const auto nodeStr  = tree[Tags::node].toString();
    Node newNode;
    if (nodeStr.isNotEmpty())
    {
        const Uuid gid (nodeStr);
        newNode = session->findNodeById (gid);
    }

    if (newNode.isValid()) {
        setNode (newNode);
    } else {
        DBG("[EL] couldn't find node to to select in node editor");
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
    g.fillAll (Element::LookAndFeel::backgroundColor);
}

void NodeEditorContentView::comboBoxChanged (ComboBox*)
{
    const auto selectedNode = graph.getNode (nodesCombo.getSelectedItemIndex());
    if (selectedNode.isValid())
    {
        if (sticky)
            setNode (selectedNode);
        ViewHelpers::findContentComponent(this)->getAppController()
            .findChild<GuiController>()->selectNode (selectedNode);
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
    menuButton.setBounds (r2.withWidth(22).withX (r2.getX() + 2));
    
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
    auto *cc = ViewHelpers::findContentComponent (this);
    jassert (cc);
    auto& graphs = *cc->getAppController().findChild<GraphController>();
    setNode (graphs.getGraph().getNode (0));
}

void NodeEditorContentView::onSessionLoaded()
{
    if (auto session = ViewHelpers::getSession (this))
        setNode (session->getActiveGraph().getNode (0));
}

void NodeEditorContentView::stabilizeContent()
{
    auto *cc = ViewHelpers::findContentComponent (this);
    auto session = ViewHelpers::getSession (this);
    jassert (cc && session);
    auto& gui = *cc->getAppController().findChild<GuiController>();
    auto& graphs = *cc->getAppController().findChild<GraphController>();
    auto& sessions = *cc->getAppController().findChild<SessionController>();

    if (! selectedNodeConnection.connected())
        selectedNodeConnection = gui.nodeSelected.connect (std::bind (
            &NodeEditorContentView::stabilizeContent, this));
    if (! graphChangedConnection.connected())
        graphChangedConnection = graphs.graphChanged.connect (std::bind (
            &NodeEditorContentView::onGraphChanged, this));
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
        nodeObjectValue = node.getPropertyAsValue (Tags::object, true);
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
    GraphNodePtr object = node.getGraphNode();
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
    
    if (node.isAudioInputNode())
    {
       #if ! EL_RUNNING_AS_PLUGIN
        if (node.isChildOfRootGraph())
        {
            return new Element::AudioDeviceSelectorComponent (world->getDeviceManager(), 
                1, DeviceManager::maxAudioChannels, 0, 0, 
                false, false, false, false);
        }
        else
        {
            return nullptr;
        }
       #else
        return new AudioIONodeEditor (node, world->getDeviceManager(), true, false);
       #endif
    }

    if (node.isAudioOutputNode())
    {
       #if ! EL_RUNNING_AS_PLUGIN
        if (node.isChildOfRootGraph())
        {
            return new Element::AudioDeviceSelectorComponent (world->getDeviceManager(), 
                0, 0, 1, DeviceManager::maxAudioChannels, 
                false, false, false, false);
        }
        else
        {
            return nullptr;
        }
       #else
        return new AudioIONodeEditor (node, world->getDeviceManager(), false, true);
       #endif
    }

    if (node.isMidiInputNode())
    {
        if (node.isChildOfRootGraph())
        {
            return new MidiIONodeEditor (node, world->getMidiEngine(), true, false);

        }
        else
        {
            return nullptr;
        }
    }

    if (node.isMidiOutputNode())
    {
        if (node.isChildOfRootGraph())
        {
            return new MidiIONodeEditor (node, world->getMidiEngine(), false, true);
        }
        else
        {
            return nullptr;
        }
    }

    GraphNodePtr object = node.getGraphNode();
    auto* const proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
    if (proc != nullptr)
    {
        if (node.getFormat() == "Element" && proc->hasEditor())
            return proc->createEditor();
        return new GenericNodeEditor (node);
    }
    else if (node.getIdentifier() == EL_INTERNAL_ID_MIDI_PROGRAM_MAP)
    {
        auto* const programChangeMapEditor = new MidiProgramMapEditor (node);
        programChangeMapEditor->setStoreSize (false);
        return programChangeMapEditor;
    }
    else if (node.getIdentifier() == EL_INTERNAL_ID_AUDIO_ROUTER)
    {
        auto* const audioRouterEditor = new AudioRouterEditor (node);
        return audioRouterEditor;
    }

    return nullptr;
}

}
