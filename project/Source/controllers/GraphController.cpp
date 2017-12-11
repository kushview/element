
#include "controllers/GraphController.h"
#include "engine/PlaceholderProcessor.h"
#include "session/PluginManager.h"

namespace Element {

GraphController::GraphController (GraphProcessor& pg, PluginManager& pm)
    : pluginManager (pm), processor (pg), lastUID (0)
{ }

GraphController::~GraphController() { }

uint32 GraphController::getNextUID() noexcept
{
    return ++lastUID;
}

int GraphController::getNumFilters() const noexcept
{
    return processor.getNumNodes();
}

const GraphNodePtr GraphController::getNode (const int index) const noexcept
{
    return processor.getNode (index);
}

const GraphNodePtr GraphController::getNodeForId (const uint32 uid) const noexcept
{
    return processor.getNodeForId (uid);
}

GraphNode* GraphController::createFilter (const PluginDescription* desc, double x, double y, uint32 nodeId)
{
    String errorMessage;
    auto* instance = pluginManager.createAudioPlugin (*desc, errorMessage);
    GraphNode* node = nullptr;
    
    if (instance != nullptr)
    {
        instance->enableAllBuses();
        
        AudioProcessor::BusesLayout stereoLayout;
        stereoLayout.inputBuses.add (AudioChannelSet::stereo());
        stereoLayout.outputBuses.add (AudioChannelSet::stereo());
        if (instance->checkBusesLayoutSupported(stereoLayout))
            instance->setBusesLayout (stereoLayout);
        
        node = processor.addNode (instance, nodeId);
    }
    
    return node;
}

GraphNode* GraphController::createPlaceholder (const Node& node)
{
    PluginDescription desc; node.getPluginDescription (desc);
    auto* ph = new PlaceholderProcessor ();
    ph->setupFor (node, 44100.0, 1024);
    return processor.addNode (ph, node.getNodeId());
}
    
uint32 GraphController::addNode (const Node& newNode)
{
    if (! newNode.isValid())
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, TRANS ("Couldn't create filter"),
                                     "Cannot instantiate node without a description");
        return KV_INVALID_NODE;
    }
    
    uint32 nodeId = KV_INVALID_NODE;
    PluginDescription desc; newNode.getPluginDescription (desc);
    
    if (auto* node = createFilter (&desc, 0, 0))
    {
        nodeId = node->nodeId;
        ValueTree data = newNode.getValueTree().createCopy();
        data.setProperty (Tags::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Tags::object, node, nullptr);
        
        data.removeProperty ("relativeX", nullptr);
        data.removeProperty ("relativeY", nullptr);
        data.removeProperty ("windowX", nullptr);
        data.removeProperty ("windowY", nullptr);
        data.removeProperty ("windowVisible", nullptr);

        const Node n (data, false);
        
        {
            MemoryBlock state;
            if (state.fromBase64Encoding (data.getProperty(Tags::state).toString()))
                node->getAudioProcessor()->setStateInformation (state.getData(), (int) state.getSize());
        }
        
        node->getAudioProcessor()->suspendProcessing (n.isBypassed());
        nodes.addChild (data, -1, nullptr);
        changed();
    }
    else
    {
        nodeId = KV_INVALID_NODE;
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, "Couldn't create filter",
                                     "The plugin could not be instantiated");
    }
    
    return nodeId;
}
    
uint32 GraphController::addFilter (const PluginDescription* desc, double rx, double ry, uint32 nodeId)
{
    if (! desc)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     TRANS ("Couldn't create filter"),
                                     TRANS ("Cannot instantiate plugin without a description"));
        return KV_INVALID_NODE;
    }

    if (auto* node = createFilter (desc, rx, ry, nodeId))
    {
        nodeId = node->nodeId;
        
        ValueTree model = node->getMetadata().createCopy();
        model.setProperty (Tags::object, node, nullptr)
             .setProperty ("relativeX", rx, 0)
             .setProperty ("relativeY", ry, 0);
        
        const Node n (model, false);
        node->getAudioProcessor()->suspendProcessing (n.isBypassed());
        nodes.addChild (model, -1, nullptr);
        changed();
    }
    else
    {
        nodeId = KV_INVALID_NODE;
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, "Couldn't create filter",
                                     "The plugin could not be instantiated");
    }

    return nodeId;
}

void GraphController::removeFilter (const uint32 uid)
{
    if (! processor.removeNode (uid))
        return;
    
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        const Node node (nodes.getChild(i), false);
        if (node.getNodeId() == uid)
        {
            nodes.removeChild (node.getValueTree(), nullptr);
        }
    }
    
    jassert(nodes.getNumChildren() == getNumFilters());
    processorArcsChanged();
}

void GraphController::disconnectFilter (const uint32 id)
{
    if (processor.disconnectNode (id))
        processorArcsChanged();
}

void GraphController::removeIllegalConnections()
{
    if (processor.removeIllegalConnections())
        processorArcsChanged();
}

int GraphController::getNumConnections() const noexcept
{
    jassert(arcs.getNumChildren() == processor.getNumConnections());
    return arcs.getNumChildren();
}

const GraphProcessor::Connection* GraphController::getConnection (const int index) const noexcept
{
    return processor.getConnection (index);
}

const GraphProcessor::Connection* GraphController::getConnectionBetween (uint32 sourceFilterUID, int sourceFilterChannel,
                                                                          uint32 destFilterUID, int destFilterChannel) const noexcept
{
    return processor.getConnectionBetween (sourceFilterUID, sourceFilterChannel,
                                           destFilterUID, destFilterChannel);
}

bool GraphController::canConnect (uint32 sourceFilterUID, int sourceFilterChannel,
                                  uint32 destFilterUID, int destFilterChannel) const noexcept
{
    return processor.canConnect (sourceFilterUID, sourceFilterChannel,
                                 destFilterUID, destFilterChannel);
}

bool GraphController::addConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                                     uint32 destFilterUID, int destFilterChannel)
{
    const bool result = processor.addConnection (sourceFilterUID, (uint32)sourceFilterChannel,
                                                 destFilterUID, (uint32)destFilterChannel);
    if (result)
        processorArcsChanged();

    return result;
}

void GraphController::removeConnection (const int index)
{
    processor.removeConnection (index);
    processorArcsChanged();
}

void GraphController::removeConnection (uint32 sourceNode, uint32 sourcePort,
                                        uint32 destNode, uint32 destPort)
{
    if (processor.removeConnection (sourceNode, sourcePort, destNode, destPort))
        processorArcsChanged();
}

void GraphController::setNodeModel (const Node& node)
{
    loaded = false;
    processor.clear();
    graph   = node.getValueTree();
    arcs    = node.getArcsValueTree();
    nodes   = node.getNodesValueTree();
    
    Array<ValueTree> failed;
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        Node node (nodes.getChild (i), false);
        PluginDescription desc; node.getPluginDescription (desc);
        if (GraphNodePtr obj = createFilter (&desc, 0.0, 0.0, node.getNodeId()))
        {
            MemoryBlock state;
            if (state.fromBase64Encoding (node.node().getProperty(Tags::state).toString()))
                obj->getAudioProcessor()->setStateInformation (state.getData(), (int)state.getSize());
            node.getValueTree().setProperty (Tags::object, obj.get(), nullptr);
            if (node.getValueTree().getProperty (Tags::bypass, false))
                obj->getAudioProcessor()->suspendProcessing (true);
        }
        else if (GraphNodePtr obj = createPlaceholder (node))
        {
            DBG("[EL] couldn't create node: " << node.getName() << ". Creating placeholder");
            node.getValueTree().setProperty (Tags::object, obj.get(), nullptr);
            node.getValueTree().setProperty ("placeholder", obj.get(), nullptr);
        }
        else
        {
            DBG("[EL] couldn't create node: " << node.getName());
            failed.add (node.getValueTree());
        }
    }
    for (const auto& n : failed)
        nodes.removeChild (n, nullptr);
    
    jassert (nodes.getNumChildren() == processor.getNumNodes());
    
    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        const ValueTree arc (arcs.getChild (i));
        bool worked = processor.addConnection ((uint32)(int) arc.getProperty (Tags::sourceNode),
                                 (uint32)(int) arc.getProperty (Tags::sourcePort),
                                 (uint32)(int) arc.getProperty (Tags::destNode),
                                 (uint32)(int) arc.getProperty (Tags::destPort));
        if (! worked)
        {
            DBG("failed connection");
        }
    }
    
    jassert (arcs.getNumChildren() == processor.getNumConnections());
    loaded = true;
    processorArcsChanged();
}

void GraphController::savePluginStates()
{
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        Node node (nodes.getChild (i), false);
        node.savePluginState();
    }
}

void GraphController::clear()
{
    loaded = false;

    processor.clear();
    
    if (graph.isValid())
    {
        graph.removeChild (arcs, nullptr);
        graph.removeChild (nodes, nullptr);
        nodes.removeAllChildren (nullptr);
        arcs.removeAllChildren (nullptr);
        graph.addChild (nodes, -1, nullptr);
        graph.addChild (arcs, -1, nullptr);
    }
    
    changed();
}

void GraphController::processorArcsChanged()
{
    ValueTree newArcs = ValueTree (Tags::arcs);
    for (int i = 0; i < processor.getNumConnections(); ++i)
        newArcs.addChild (Node::makeArc (*processor.getConnection(i)), -1, nullptr);
    const auto index = graph.indexOf (arcs);
    graph.removeChild (arcs, nullptr);
    graph.addChild (newArcs, index, nullptr);
    arcs = graph.getChildWithName (Tags::arcs);
    changed();
}

// MARK: Root Graph Controller
void RootGraphController::unloadGraph()
{
    getRootGraph().clear();
}
    
}
