
#include "controllers/GraphController.h"
#include "engine/PlaceholderProcessor.h"
#include "engine/SubGraphProcessor.h"
#include "session/PluginManager.h"
#include "session/UnlockStatus.h"
#include "Globals.h"

namespace Element {

static StringArray getFullVesrionPluginIdentifiers() {
return { "element.audioMixer",
         "element.graph",
         "element.placeholder",
         "element." };
}

static bool isFullVersionPlugin (const PluginDescription& desc) {
    return getFullVesrionPluginIdentifiers().contains (desc.fileOrIdentifier);
}

static void showFailedInstantiationAlert (const PluginDescription& desc, const bool async = false)
{
    String header = "Plugin Instantiation Failed";
    String message;
    if (isFullVersionPlugin (desc))
        message << desc.name << " is available in the pro version.";
    else
        message << desc.name << " could not be instantiated";

    if (async)
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, header, message);
    else
        AlertWindow::showMessageBox (AlertWindow::WarningIcon, header, message);
}

/** This enforces correct IO nodes based on the graph processor's settings
    in virtual methods like 'acceptsMidi' and 'getTotalNumInputChannels'
    It uses the controller for all node operations so the model will
    stay in sync.
  */
struct IONodeEnforcer
{
    IONodeEnforcer (GraphController& c)
        : controller (c)
    {
        addMissingIONodes(); 
        controller.syncArcsModel();
    }
    
    GraphController* getController() const { return &controller; }

private:
    GraphController& controller;
    void addMissingIONodes()
    {
        auto* root = getController();
        if (! root) return;
        auto& proc (root->getGraph());
        const Node graph (root->getGraphModel());

        const bool wantsAudioIn   = graph.hasAudioInputNode()  && proc.getTotalNumInputChannels() > 0;
        const bool wantsAudioOut  = graph.hasAudioOutputNode() && proc.getTotalNumOutputChannels() > 0;
        const bool wantsMidiIn    = graph.hasMidiInputNode()   && proc.acceptsMidi();
        const bool wantsMidiOut   = graph.hasMidiOutputNode()  && proc.producesMidi();
        
        GraphNodePtr ioNodes [IOProcessor::numDeviceTypes];
        for (int i = 0; i < root->getNumFilters(); ++i)
        {
            GraphNodePtr node = root->getNode (i);
            if (node->isMidiIONode() || node->isAudioIONode())
            {
                auto* ioProc = dynamic_cast<IOProcessor*> (node->getAudioProcessor());
                ioNodes [ioProc->getType()] = node;
            }
        }
        
        Array<uint32> nodesToRemove;

        for (int t = 0; t < IOProcessor::numDeviceTypes; ++t)
        {
            if (nullptr != ioNodes [t])
            {
                if (t == IOProcessor::audioInputNode    && !wantsAudioIn)   nodesToRemove.add (ioNodes[t]->nodeId);
                if (t == IOProcessor::audioOutputNode   && !wantsAudioOut)  nodesToRemove.add (ioNodes[t]->nodeId);;
                if (t == IOProcessor::midiInputNode     && !wantsMidiIn)    nodesToRemove.add (ioNodes[t]->nodeId);;
                if (t == IOProcessor::midiOutputNode    && !wantsMidiOut)   nodesToRemove.add (ioNodes[t]->nodeId);;
                continue;
            }

            if (t == IOProcessor::audioInputNode    && !wantsAudioIn)   continue;
            if (t == IOProcessor::audioOutputNode   && !wantsAudioOut)  continue;
            if (t == IOProcessor::midiInputNode     && !wantsMidiIn)    continue;
            if (t == IOProcessor::midiOutputNode    && !wantsMidiOut)   continue;

            PluginDescription desc;
            desc.pluginFormatName = "Internal";
            double rx = 0.5f, ry = 0.5f;
            switch (t)
            {
                case IOProcessor::audioInputNode:
                    desc.fileOrIdentifier = "audio.input";
                    rx = .25;
                    ry = .25;
                    break;
                case IOProcessor::audioOutputNode:
                    desc.fileOrIdentifier = "audio.output";
                    rx = .25;
                    ry = .75;
                    break;
                case IOProcessor::midiInputNode:
                    desc.fileOrIdentifier = "midi.input";
                    rx = .75;
                    ry = .25;
                    break;
                case IOProcessor::midiOutputNode:
                    desc.fileOrIdentifier = "midi.output";
                    rx = .75;
                    ry = .75;
                    break;
            }
            
            auto nodeId = root->addFilter (&desc, rx, ry);
            ioNodes[t] = root->getNodeForId (nodeId);
            jassert(ioNodes[t] != nullptr);
        }

        for (const auto& nodeId : nodesToRemove)
            root->removeFilter (nodeId);
    }
};

GraphController::GraphController (GraphProcessor& pg, PluginManager& pm)
    : pluginManager (pm), processor (pg), lastUID (0)
{ }

GraphController::~GraphController()
{
    // Make sure to dereference GraphNode's so we don't leak memory
    // If you get warnings by juce's leak detector about graph related
    // objects, then there's probably "object" properties lingering that
    // are referenced in the model;
    Node::sanitizeRuntimeProperties (graph, true);
    graph = arcs = nodes = ValueTree();
}

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
        if (auto* sub = dynamic_cast<SubGraphProcessor*> (instance))
            sub->initController (pluginManager);
        instance->enableAllBuses();
        node = processor.addNode (instance, nodeId);
    }
    
    if (errorMessage.isNotEmpty())
    {
        DBG("[EL] error creating audio plugin: " << errorMessage);
    }
    
    return node;
}

GraphNode* GraphController::createPlaceholder (const Node& node)
{
    PluginDescription desc; node.getPluginDescription (desc);
    auto* ph = new PlaceholderProcessor ();
    ph->setupFor (node, processor.getSampleRate(), processor.getBlockSize());
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
    const PluginDescription desc (pluginManager.findDescriptionFor (newNode));
    if (auto* node = createFilter (&desc, 0, 0,
        newNode.hasProperty(Tags::id) ? newNode.getNodeId() : 0))
    {
        nodeId = node->nodeId;
        ValueTree data = newNode.getValueTree().createCopy();
        data.setProperty (Tags::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Tags::object, node, nullptr)
            .setProperty (Tags::type, node->getTypeString(), nullptr)
            .setProperty (Tags::pluginIdentifierString, desc.createIdentifierString(), nullptr);
        
        data.removeProperty (Tags::relativeX, nullptr);
        data.removeProperty (Tags::relativeY, nullptr);
        data.removeProperty (Tags::windowX, nullptr);
        data.removeProperty (Tags::windowY, nullptr);
        data.removeProperty (Tags::windowVisible, nullptr);

        setupNode (data, node);

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
        auto* const proc = node->getAudioProcessor();

        nodeId = node->nodeId;
        ValueTree model = node->getMetadata().createCopy();
        model.setProperty (Tags::object, node, nullptr)
             .setProperty (Tags::relativeX, rx, nullptr)
             .setProperty (Tags::relativeY, ry, nullptr)
             .setProperty (Tags::pluginIdentifierString, 
                           desc->createIdentifierString(), nullptr);
        
        Node n (model, true);

        if (auto* sub = dynamic_cast<SubGraphProcessor*> (node->getAudioProcessor()))
        {
            sub->getController().setNodeModel (n);
            IONodeEnforcer enforceIONodes (sub->getController());
        }
        
        {
            // try to use stereo by default on newly added plugins
            AudioProcessor::BusesLayout stereoInOut;
            stereoInOut.inputBuses.add (AudioChannelSet::stereo());
            stereoInOut.outputBuses.add (AudioChannelSet::stereo());
            AudioProcessor::BusesLayout stereoOut;
            stereoOut.outputBuses.add (AudioChannelSet::stereo());
            AudioProcessor::BusesLayout* tryStereo = nullptr;

            if (proc->getTotalNumInputChannels() == 1 &&
                proc->getTotalNumOutputChannels() == 1 &&
                proc->checkBusesLayoutSupported (stereoInOut))
            {
                tryStereo = &stereoInOut;
            }
            else if (proc->getTotalNumInputChannels() == 0 &&
                proc->getTotalNumOutputChannels() == 1 &&
                proc->checkBusesLayoutSupported (stereoOut))
            {
                tryStereo = &stereoOut;
            }

            if (tryStereo != nullptr)
            {
                proc->suspendProcessing (true);
                proc->releaseResources();
                const bool success = proc->setBusesLayout (*tryStereo);
                proc->prepareToPlay (processor.getSampleRate(), processor.getBlockSize());
                proc->suspendProcessing (false);
                DBG("[EL] attempting stereo preference: " << String (success ? "success" : "failed"));
            }
        }

        // make sure the model ports are correct with the actual processor
        n.resetPorts();

        nodes.addChild (model, -1, nullptr);
        changed();
    }
    else
    {
        nodeId = KV_INVALID_NODE;
        showFailedInstantiationAlert (*desc, true);
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
            // the model was probably referencing the node ptr
            GraphNodePtr obj = node.getGraphNode();
            auto data = node.getValueTree();
            nodes.removeChild (data, nullptr);
            // clear all referecnce counted objects
            Node::sanitizeProperties (data, true);
            // finally delete the node + plugin instance.
            obj = nullptr;
        }
    }
    
    jassert(nodes.getNumChildren() == getNumFilters());
    processorArcsChanged();
}

void GraphController::disconnectFilter (const uint32 nodeId, const bool inputs, const bool outputs)
{
    jassert(inputs || outputs);
    bool doneAnything = false;

    for (int i = getNumConnections(); --i >= 0;)
    {
        const auto* const c = processor.getConnection (i);
        if ((outputs && c->sourceNode == nodeId) || 
            (inputs && c->destNode == nodeId))
        {
            removeConnection (i);
            doneAnything = true;
        }
    }

    if (doneAnything)
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
    {
        processorArcsChanged();
    }
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
        const PluginDescription desc (pluginManager.findDescriptionFor (node));
        if (GraphNodePtr obj = createFilter (&desc, 0.0, 0.0, node.getNodeId()))
        {
            setupNode (node.getValueTree(), obj);
            obj->setEnabled (node.isEnabled());
            node.setProperty (Tags::enabled, obj->isEnabled());
        }
        else if (GraphNodePtr ph = createPlaceholder (node))
        {
            DBG("[EL] couldn't create node: " << node.getName() << ". Creating offline placeholder");
            node.getValueTree().setProperty (Tags::object, ph.get(), nullptr);
            node.getValueTree().setProperty (Tags::missing, true, nullptr);
        }
        else
        {
            DBG("[EL] couldn't create node: " << node.getName());
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
    
    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        ValueTree arc (arcs.getChild (i));
        bool worked = processor.addConnection ((uint32)(int) arc.getProperty (Tags::sourceNode),
                                               (uint32)(int) arc.getProperty (Tags::sourcePort),
                                               (uint32)(int) arc.getProperty (Tags::destNode),
                                               (uint32)(int) arc.getProperty (Tags::destPort));
        if (worked)
        {
            arc.removeProperty (Tags::missing, 0);
        }
        else
        {
            DBG("[EL] failed creating connection");
            arc.setProperty (Tags::missing, true, 0);
            failed.add (arc);
        }
    }

    const bool discardFailedConnections = false;
    if (discardFailedConnections)
    {
        for (const auto& n : failed)
        {
            arcs.removeChild (n, nullptr);
            Node::sanitizeRuntimeProperties (n);
        }
    }

    loaded = true;
    jassert (arcs.getNumChildren() - failed.size() == processor.getNumConnections());
    failed.clearQuick();

    IONodeEnforcer enforceIONodes (*this);
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

void GraphController::processorArcsChanged()
{
    ValueTree newArcs = ValueTree (Tags::arcs);
    for (int i = 0; i < processor.getNumConnections(); ++i)
        newArcs.addChild (Node::makeArc (*processor.getConnection (i)), -1, nullptr);

    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        const ValueTree arc (arcs.getChild (i));
        if (true == (bool) arc [Tags::missing])
        {   
            ValueTree missingArc = arc.createCopy();
            if (processor.addConnection (
                (uint32)(int) missingArc[Tags::sourceNode],
                (uint32)(int) missingArc[Tags::sourcePort],
                (uint32)(int) missingArc[Tags::destNode],
                (uint32)(int) missingArc[Tags::destPort]))
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

void GraphController::setupNode (const ValueTree& data, GraphNodePtr obj)
{
    jassert (obj && data.hasType (Tags::node));
    Node node (data, false);
    node.getValueTree().setProperty (Tags::object, obj.get(), nullptr);
    auto* const proc = obj->getAudioProcessor();
    PortArray ins, outs;
    node.getPorts (ins, outs, PortType::Audio);

    // try to match ports
    if (proc->getTotalNumInputChannels() != ins.size() ||
        proc->getTotalNumOutputChannels() != outs.size())
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
        
        node.resetPorts();
    }
    
    if (auto* sub = dynamic_cast<SubGraphProcessor*> (proc))
    {
        sub->getController().setNodeModel (node);
        node.resetPorts();
    }
    
    node.restorePluginState();
}

// MARK: Root Graph Controller
void RootGraphController::unloadGraph()
{
    getRootGraph().clear();
}

}
