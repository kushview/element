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

#include "engine/graphmanager.hpp"
#include "engine/nodes/AudioProcessorNode.h"
#include "engine/nodes/AudioRouterNode.h"
#include "engine/nodes/MidiChannelSplitterNode.h"
#include "engine/nodes/MidiProgramMapNode.h"
#include "engine/nodes/PlaceholderProcessor.h"
#include "engine/rootgraph.hpp"
#include <element/pluginmanager.hpp>
#include <element/context.hpp>
#include "utils.hpp"

namespace element {

//==============================================================================
static void showFailedInstantiationAlert (const PluginDescription& desc, const bool async = false)
{
    String header = "Plugin Instantiation Failed";
    String message = desc.name;
    message << " could not be instantiated";

    if (async)
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, header, message);
    else
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, header, message);
}

//==============================================================================
static void removeCoordinatesProperties (ValueTree data)
{
    data.removeProperty (Tags::relativeX, nullptr);
    data.removeProperty (Tags::relativeY, nullptr);
}

static void removeWindowProperties (ValueTree data)
{
    data.removeProperty (Tags::windowX, nullptr);
    data.removeProperty (Tags::windowY, nullptr);
    data.removeProperty (Tags::windowVisible, nullptr);
}

//==============================================================================
/** This enforces correct IO nodes based on the graph processor's settings
    in virtual methods like 'acceptsMidi' and 'getTotalNumInputChannels'
    It uses the controller for all node operations so the model will
    stay in sync.
  */
struct IONodeEnforcer
{
    IONodeEnforcer (GraphManager& m)
        : manager (m)
    {
        addMissingIONodes();
    }

private:
    GraphManager& manager;
    void addMissingIONodes()
    {
        auto& graph (manager.getGraph());
        const Node model (manager.getGraphModel());

        const bool wantsAudioIn = graph.getNumPorts (PortType::Audio, true) > 0 && model.hasAudioInputNode();
        const bool wantsAudioOut = graph.getNumPorts (PortType::Audio, false) > 0 && model.hasAudioOutputNode();
        const bool wantsMidiIn = graph.getNumPorts (PortType::Midi, true) > 0 && model.hasMidiInputNode();
        const bool wantsMidiOut = graph.getNumPorts (PortType::Midi, false) > 0 && model.hasMidiOutputNode();

        NodeObjectPtr ioNodes[IONode::numDeviceTypes];
        for (int i = 0; i < manager.getNumNodes(); ++i)
        {
            NodeObjectPtr node = manager.getNode (i);
            if (auto* ioProc = dynamic_cast<IONode*> (node.get()))
                ioNodes[ioProc->getType()] = node;
        }

        Array<uint32> nodesToRemove;

        for (int t = 0; t < IONode::numDeviceTypes; ++t)
        {
            if (nullptr != ioNodes[t])
            {
                if (t == IONode::audioInputNode && ! wantsAudioIn)
                    nodesToRemove.add (ioNodes[t]->nodeId);
                if (t == IONode::audioOutputNode && ! wantsAudioOut)
                    nodesToRemove.add (ioNodes[t]->nodeId);
                ;
                if (t == IONode::midiInputNode && ! wantsMidiIn)
                    nodesToRemove.add (ioNodes[t]->nodeId);
                ;
                if (t == IONode::midiOutputNode && ! wantsMidiOut)
                    nodesToRemove.add (ioNodes[t]->nodeId);
                ;
                continue;
            }

            if (t == IONode::audioInputNode && ! wantsAudioIn)
                continue;
            if (t == IONode::audioOutputNode && ! wantsAudioOut)
                continue;
            if (t == IONode::midiInputNode && ! wantsMidiIn)
                continue;
            if (t == IONode::midiOutputNode && ! wantsMidiOut)
                continue;

            PluginDescription desc;
            desc.pluginFormatName = "Internal";
            double rx = 0.5f, ry = 0.5f;
            switch (t)
            {
                case IONode::audioInputNode:
                    desc.fileOrIdentifier = "audio.input";
                    rx = .25;
                    ry = .25;
                    break;
                case IONode::audioOutputNode:
                    desc.fileOrIdentifier = "audio.output";
                    rx = .25;
                    ry = .75;
                    break;
                case IONode::midiInputNode:
                    desc.fileOrIdentifier = "midi.input";
                    rx = .75;
                    ry = .25;
                    break;
                case IONode::midiOutputNode:
                    desc.fileOrIdentifier = "midi.output";
                    rx = .75;
                    ry = .75;
                    break;
            }

            auto nodeId = manager.addNode (&desc, rx, ry);
            ioNodes[t] = manager.getNodeForId (nodeId);
            jassert (ioNodes[t] != nullptr);
        }

        for (const auto& nodeId : nodesToRemove)
            manager.removeNode (nodeId);
    }
};

//==============================================================================
class NodeModelUpdater : public ReferenceCountedObject
{
public:
    NodeModelUpdater (GraphManager& m, const ValueTree& d, NodeObject* o)
        : manager (m), data (d), object (o)
    {
        portsChangedConnection = object->portsChanged.connect (
            std::bind (&NodeModelUpdater::onPortsChanged, this));
    }

    ~NodeModelUpdater()
    {
        portsChangedConnection.disconnect();
    }

private:
    GraphManager& manager;
    ValueTree data;
    NodeObjectPtr object;
    SignalConnection portsChangedConnection;

    void onPortsChanged()
    {
        const auto newPorts = object->createPortsData();
        int index = data.indexOf (data.getChildWithName (Tags::ports));
        if (newPorts.isValid())
        {
            if (index >= 0)
                data.removeChild (index, nullptr);
            else
                index = -1;

            data.addChild (newPorts, index, nullptr);
            manager.removeIllegalConnections();
        }

        manager.syncArcsModel();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeModelUpdater);
};

//==============================================================================
class GraphManager::Binding
{
public:
    Binding() = delete;
    Binding (GraphManager& g, NodeObjectPtr o, const Node& n)
        : owner (g),
          object (o),
          node (n)
    {
        data = node.getValueTree();
        setDataProperties();

        if (node.getNodeType() == Tags::graph && object && object->isGraph())
        {
            auto sub = dynamic_cast<GraphNode*> (object.get());
            jassert (sub);
            manager = std::make_unique<GraphManager> (*sub, owner.pluginManager);
            manager->setNodeModel (node);
            IONodeEnforcer addIO (*manager);
        }
        else
        {
            if (node.getNodeType() != Tags::graph)
            {
                DBG ("data type is not a graph");
            }
            if (object && ! object->isGraph())
            {
                DBG ("object is not a graph type");
            }
        }
    }

    ~Binding()
    {
        if (manager != nullptr)
        {
            manager.reset();
        }

        node = Node();
        Node::sanitizeRuntimeProperties (data);
        data = ValueTree();
        object = nullptr;
    }

    GraphManager* getSubGraphManager() const { return manager.get(); }

    void setDataProperties()
    {
        if (! data.isValid() || object == nullptr)
            return;
        data.setProperty (Tags::id, static_cast<int64> (object->nodeId), nullptr)
            .setProperty (Tags::object, object.get(), nullptr)
            .setProperty (Tags::name, object->getName(), nullptr);
    }

private:
    friend class GraphManager;
    GraphManager& owner;
    NodeObjectPtr object;
    Node node;
    ValueTree data;
    std::unique_ptr<GraphManager> manager;
    UndoManager* undo = nullptr;
};

//==============================================================================
GraphManager::GraphManager (GraphNode& pg, PluginManager& pm)
    : pluginManager (pm), processor (pg), lastUID (0)
{
}

GraphManager::~GraphManager()
{
    // Make sure to dereference NodeObject's so we don't leak memory
    // If you get warnings by juce's leak detector about graph related
    // objects, then there's probably "object" properties lingering that
    // are referenced in the model;
    Node::sanitizeRuntimeProperties (graph, true);
    graph = arcs = nodes = ValueTree();
}

uint32 GraphManager::getNextUID() noexcept
{
    return ++lastUID;
}

int GraphManager::getNumNodes() const noexcept { return processor.getNumNodes(); }
const NodeObjectPtr GraphManager::getNode (const int index) const noexcept { return processor.getNode (index); }

const NodeObjectPtr GraphManager::getNodeForId (const uint32 uid) const noexcept
{
    return processor.getNodeForId (uid);
}

const Node GraphManager::getNodeModelForId (const uint32 nodeId) const noexcept
{
    return Node (nodes.getChildWithProperty (Tags::id, static_cast<int64> (nodeId)), false);
}

GraphManager* GraphManager::findGraphManagerForGraph (const Node& graph) const noexcept
{
    if (isManaging (graph))
        return const_cast<GraphManager*> (this);

    for (auto* binding : bindings)
    {
        if (auto* m1 = binding->getSubGraphManager())
            if (auto* m2 = m1->findGraphManagerForGraph (graph))
                return m2;
    }

    return nullptr;
}

bool GraphManager::contains (const uint32 nodeId) const
{
    return processor.getNodeForId (nodeId) != nullptr;
}

NodeObject* GraphManager::createFilter (const PluginDescription* desc, double x, double y, uint32 nodeId)
{
    String errorMessage;
    auto node = std::unique_ptr<NodeObject> (
        pluginManager.createGraphNode (*desc, errorMessage));

    if (errorMessage.isNotEmpty())
    {
        DBG ("[EL] error creating audio plugin: " << errorMessage);
        jassert (node == nullptr);
    }

    if (node == nullptr && errorMessage.isEmpty())
    {
        jassertfalse;
        errorMessage = "Could not find node";
    }

    return node != nullptr ? processor.addNode (node.release(), nodeId) : nullptr;
}

NodeObject* GraphManager::createPlaceholder (const Node& node)
{
    auto* ph = new PlaceholderProcessor();
    ph->setupFor (node, processor.getSampleRate(), processor.getBlockSize());
    return processor.addNode (new AudioProcessorNode (node.getNodeId(), ph), node.getNodeId());
}

//==============================================================================
uint32 GraphManager::addNode (const Node& newNode)
{
    if (! newNode.isValid())
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, TRANS ("Couldn't create Node"), "Cannot instantiate node without a description");
        return EL_INVALID_NODE;
    }

    uint32 nodeId = EL_INVALID_NODE;
    const PluginDescription desc (pluginManager.findDescriptionFor (newNode));
    if (auto* node = createFilter (&desc, 0, 0, newNode.hasProperty (Tags::id) ? newNode.getNodeId() : 0))
    {
        nodeId = node->nodeId;
        ValueTree data = newNode.getValueTree().createCopy();
        removeCoordinatesProperties (data);
        removeWindowProperties (data);

        data.setProperty (Tags::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Tags::object, node, nullptr)
            .setProperty (Tags::type, node->getTypeString(), nullptr)
            .setProperty (Tags::pluginIdentifierString, desc.createIdentifierString(), nullptr);

        setupNode (data, node);

        nodes.addChild (data, -1, nullptr);
        changed();
    }
    else
    {
        nodeId = EL_INVALID_NODE;
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, "Couldn't create filter", "The plugin could not be instantiated");
    }

    return nodeId;
}

uint32 GraphManager::addNode (const PluginDescription* desc, double rx, double ry, uint32 nodeId)
{
    if (! desc)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     TRANS ("Couldn't create filter"),
                                     TRANS ("Cannot instantiate plugin without a description"));
        return EL_INVALID_NODE;
    }

    if (auto* object = createFilter (desc, rx, ry, nodeId))
    {
        nodeId = object->nodeId;
        ValueTree data = ! object->isGraph() ? ValueTree (Tags::node)
                                             : Node::createDefaultGraph (desc->name).getValueTree();

        data.setProperty (Tags::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Tags::format, desc->pluginFormatName, nullptr)
            .setProperty (Tags::identifier, desc->fileOrIdentifier, nullptr)
            .setProperty (Tags::type, object->getTypeString(), nullptr)
            .setProperty (Tags::name, desc->name, nullptr)
            .setProperty (Tags::object, object, nullptr)
            .setProperty (Tags::updater, new NodeModelUpdater (*this, data, object), nullptr)
            .setProperty (Tags::relativeX, rx, nullptr)
            .setProperty (Tags::relativeY, ry, nullptr)
            .setProperty (Tags::pluginIdentifierString,
                          desc->createIdentifierString(),
                          nullptr);

        Node node (data, true);
        jassert (node.getFormat().toString().isNotEmpty());
        jassert (node.getIdentifier().toString().isNotEmpty());

        if (object->isSubGraph())
        {
            bindings.add (new Binding (*this, object, node));
        }

        if (auto* const proc = object->getAudioProcessor())
        {
            // try to use stereo by default on newly added plugins
            AudioProcessor::BusesLayout stereoInOut;
            stereoInOut.inputBuses.add (AudioChannelSet::stereo());
            stereoInOut.outputBuses.add (AudioChannelSet::stereo());
            AudioProcessor::BusesLayout stereoOut;
            stereoOut.outputBuses.add (AudioChannelSet::stereo());
            AudioProcessor::BusesLayout* tryStereo = nullptr;
            const auto oldLayout = proc->getBusesLayout();

            if (proc->getTotalNumInputChannels() == 1 && proc->getTotalNumOutputChannels() == 1 && proc->checkBusesLayoutSupported (stereoInOut))
            {
                tryStereo = &stereoInOut;
            }
            else if (proc->getTotalNumInputChannels() == 0 && proc->getTotalNumOutputChannels() == 1 && proc->checkBusesLayoutSupported (stereoOut))
            {
                tryStereo = &stereoOut;
            }

            if (tryStereo != nullptr && proc->checkBusesLayoutSupported (*tryStereo))
            {
                proc->suspendProcessing (true);
                proc->releaseResources();

                if (! proc->setBusesLayout (*tryStereo))
                    proc->setBusesLayout (oldLayout);

                proc->prepareToPlay (processor.getSampleRate(), processor.getBlockSize());
                proc->suspendProcessing (false);
            }
        }

        // make sure the model ports are correct with the actual processor
        node.resetPorts();
        nodes.addChild (data, -1, nullptr);
        changed();
    }
    else
    {
        nodeId = EL_INVALID_NODE;
        showFailedInstantiationAlert (*desc, true);
    }

    return nodeId;
}

void GraphManager::removeNode (const uint32 uid)
{
    if (! processor.removeNode (uid))
        return;
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        const Node node (nodes.getChild (i), false);
        if (node.getNodeId() == uid)
        {
            // the model was probably referencing the node ptr
            NodeObjectPtr obj = node.getObject();
            if (obj)
            {
                obj->willBeRemoved();
                obj->releaseResources();
            }

            for (int i = bindings.size(); --i >= 0;)
            {
                auto binding = bindings.getUnchecked (i);
                if (binding->object == obj)
                    bindings.remove (i, true);
            }

            auto data = node.getValueTree();
            nodes.removeChild (data, nullptr);
            // clear all referecnce counted objects
            Node::sanitizeProperties (data, true);
            // finally delete the node + plugin instance.
            obj = nullptr;
        }
    }

    jassert (nodes.getNumChildren() == getNumNodes());
    processorArcsChanged();
}

//==============================================================================
void GraphManager::disconnectNode (const uint32 nodeId, const bool inputs, const bool outputs, const bool audio, const bool midi)
{
    jassert (inputs || outputs);
    bool doneAnything = false;

    for (int i = getNumConnections(); --i >= 0;)
    {
        const auto* const c = processor.getConnection (i);
        if ((outputs && c->sourceNode == nodeId) || (inputs && c->destNode == nodeId))
        {
            NodeObjectPtr src = processor.getNodeForId (c->sourceNode);
            NodeObjectPtr dst = processor.getNodeForId (c->destNode);

            if ((audio && src->getPortType (c->sourcePort) == PortType::Audio && dst->getPortType (c->destPort) == PortType::Audio) || (midi && src->getPortType (c->sourcePort) == PortType::Midi && dst->getPortType (c->destPort) == PortType::Midi))
            {
                removeConnection (i);
                doneAnything = true;
            }
        }
    }

    if (doneAnything)
        processorArcsChanged();
}

void GraphManager::removeIllegalConnections()
{
    if (processor.removeIllegalConnections())
        processorArcsChanged();
}

int GraphManager::getNumConnections() const noexcept
{
    jassert (arcs.getNumChildren() == processor.getNumConnections());
    return processor.getNumConnections();
}

const GraphNode::Connection* GraphManager::getConnection (const int index) const noexcept
{
    return processor.getConnection (index);
}

const GraphNode::Connection* GraphManager::getConnectionBetween (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel) const noexcept
{
    return processor.getConnectionBetween (sourceFilterUID, sourceFilterChannel, destFilterUID, destFilterChannel);
}

bool GraphManager::canConnect (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel) const noexcept
{
    return processor.canConnect (sourceFilterUID, sourceFilterChannel, destFilterUID, destFilterChannel);
}

bool GraphManager::addConnection (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel)
{
    const bool result = processor.addConnection (sourceFilterUID, (uint32) sourceFilterChannel, destFilterUID, (uint32) destFilterChannel);
    if (result)
        processorArcsChanged();

    return result;
}

void GraphManager::removeConnection (const int index)
{
    processor.removeConnection (index);
    processorArcsChanged();
}

void GraphManager::removeConnection (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort)
{
    if (processor.removeConnection (sourceNode, sourcePort, destNode, destPort))
        processorArcsChanged();
}

void GraphManager::setNodeModel (const Node& node)
{
    loaded = false;

    processor.clear();
    graph = node.getValueTree();
    arcs = node.getArcsValueTree();
    nodes = node.getNodesValueTree();

    Array<ValueTree> failed;
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        Node node (nodes.getChild (i), false);
        const PluginDescription desc (pluginManager.findDescriptionFor (node));
        if (NodeObjectPtr obj = createFilter (&desc, 0, 0, node.getNodeId()))
        {
            setupNode (node.getValueTree(), obj);
            obj->setEnabled (node.isEnabled());
            node.setProperty (Tags::enabled, obj->isEnabled());
        }
        else if (NodeObjectPtr ph = createPlaceholder (node))
        {
            DBG ("[EL] couldn't create node: " << node.getName() << ". Creating offline placeholder");
            node.getValueTree().setProperty (Tags::object, ph.get(), nullptr);
            node.getValueTree().setProperty (Tags::missing, true, nullptr);
        }
        else
        {
            DBG ("[EL] couldn't create node: " << node.getName());
            failed.add (node.getValueTree());
        }
    }

    for (const auto& n : failed)
    {
        nodes.removeChild (n, nullptr);
        Node::sanitizeRuntimeProperties (n);
    }
    failed.clearQuick();

    // If you hit this, then failed nodes didn't get handled properly
    jassert (nodes.getNumChildren() == processor.getNumNodes());

    // Cheap way to refresh engine-side nodes
    processor.triggerAsyncUpdate();
    processor.handleUpdateNowIfNeeded();

    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        ValueTree arc (arcs.getChild (i));
        const auto sourceNode = (uint32) (int) arc.getProperty (Tags::sourceNode);
        const auto destNode = (uint32) (int) arc.getProperty (Tags::destNode);
        bool worked = processor.addConnection (sourceNode, (uint32) (int) arc.getProperty (Tags::sourcePort), destNode, (uint32) (int) arc.getProperty (Tags::destPort));
        if (worked)
        {
            arc.removeProperty (Tags::missing, 0);
        }
        else
        {
            DBG ("[EL] failed creating connection: ");
            const Node graphObject (graph, false);

            if (graphObject.getNodeById (sourceNode).isValid() && graphObject.getNodeById (destNode).isValid())
            {
                DBG ("[EL] set missing connection");
                // if the nodes are valid then preserve it
                arc.setProperty (Tags::missing, true, 0);
            }
            else
            {
                DBG ("[EL] purge failed arc");
                failed.add (arc);
            }
        }
    }

    const bool discardFailedConnections = true;
    if (discardFailedConnections)
        for (const auto& n : failed)
            arcs.removeChild (n, nullptr);

    loaded = true;
    jassert (arcs.getNumChildren() == processor.getNumConnections());
    failed.clearQuick();

    IONodeEnforcer enforceIONodes (*this);
    processorArcsChanged();
}

void GraphManager::savePluginStates()
{
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        Node node (nodes.getChild (i), false);
        node.savePluginState();
    }
}

void GraphManager::clear()
{
    loaded = false;

    if (graph.isValid())
    {
        Node::sanitizeRuntimeProperties (graph);
        graph.removeChild (arcs, nullptr);
        graph.removeChild (nodes, nullptr);
        nodes.removeAllChildren (nullptr);
        arcs.removeAllChildren (nullptr);
        graph.addChild (nodes, -1, nullptr);
        graph.addChild (arcs, -1, nullptr);
    }

    processor.clear();
    changed();
}

void GraphManager::processorArcsChanged()
{
    ValueTree newArcs = ValueTree (Tags::arcs);
    for (int i = 0; i < processor.getNumConnections(); ++i)
        newArcs.addChild (Node::makeArc (*processor.getConnection (i)), -1, nullptr);

    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        const ValueTree arc (arcs.getChild (i));
        if (true == (bool) arc[Tags::missing])
        {
            ValueTree missingArc = arc.createCopy();
            if (processor.addConnection (
                    (uint32) (int) missingArc[Tags::sourceNode],
                    (uint32) (int) missingArc[Tags::sourcePort],
                    (uint32) (int) missingArc[Tags::destNode],
                    (uint32) (int) missingArc[Tags::destPort]))
            {
                missingArc.removeProperty (Tags::missing, 0);
            }

            newArcs.addChild (missingArc, -1, 0);
        }
    }

    const auto index = graph.indexOf (arcs);
    graph.removeChild (arcs, nullptr);
    graph.addChild (newArcs, index, nullptr);
    arcs = graph.getChildWithName (Tags::arcs);
    changed();
}

void GraphManager::setupNode (const ValueTree& data, NodeObjectPtr obj)
{
    jassert (obj && data.hasType (Tags::node));
    Node node (data, false);
    node.setProperty (Tags::type, obj->getTypeString())
        .setProperty (Tags::object, obj.get())
        .setProperty (Tags::updater, new NodeModelUpdater (*this, data, obj.get()));

    PortArray ins, outs;
    node.getPorts (ins, outs, PortType::Audio);
    bool resetPorts = false;
    if (auto* const proc = obj->getAudioProcessor())
    {
        // try to match ports
        if (proc->getTotalNumInputChannels() != ins.size() || proc->getTotalNumOutputChannels() != outs.size())
        {
            AudioProcessor::BusesLayout layout;
            layout.inputBuses.add (AudioChannelSet::namedChannelSet (ins.size()));
            layout.outputBuses.add (AudioChannelSet::namedChannelSet (outs.size()));

            if (proc->checkBusesLayoutSupported (layout))
            {
                proc->suspendProcessing (true);
                proc->releaseResources();
                proc->setBusesLayoutWithoutEnabling (layout);
                proc->prepareToPlay (processor.getSampleRate(), processor.getBlockSize());
                proc->suspendProcessing (false);
            }

            resetPorts = true;
        }
    }

    if (obj->isSubGraph())
    {
        bindings.add (new Binding (*this, obj, node));
        resetPorts = true;
    }

    node.restorePluginState();
    node.resetPorts();
    jassert (node.getNumPorts() == static_cast<int> (obj->getNumPorts()));
}

// MARK: Root Graph Controller
RootGraphManager::RootGraphManager (RootGraph& graph, PluginManager& plugins)
    : GraphManager (graph, plugins),
      root (graph)
{
}

RootGraphManager::~RootGraphManager() {}

void RootGraphManager::unloadGraph()
{
    getRootGraph().clear();
}

} // namespace element
