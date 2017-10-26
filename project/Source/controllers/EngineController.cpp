
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
#include "gui/PluginWindow.h"
#include "session/DeviceManager.h"
#include "session/Node.h"
#include "Globals.h"
#include "Settings.h"

#include "controllers/EngineController.h"

namespace Element {

namespace Logging
{
    void dump (GraphController& g)
    {
        DBG("[EL] graph: " << g.getGraph().getName());
        for (int i = 0; i < g.getNumFilters(); ++i)
        {
            PluginDescription d;
            g.getNode(i)->getPluginDescription (d);
            DBG("[EL] GC Node Name: " << d.name);
        }
    }
}

EngineController::EngineController() { }
EngineController::~EngineController()
{
    if (root)
    {
        root->removeChangeListener (this);
        root = nullptr;
    }
}

void EngineController::addConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    jassert (root);
    root->addConnection (s, sp, d, dp);
}

void EngineController::addGraph()
{
    auto& world  = (dynamic_cast<AppController*>(getRoot()))->getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    ScopedPointer<RootGraph> newGraph = new RootGraph();
    if (engine->addGraph (newGraph.get()))
    {
        newGraph.release();
        Node node (Tags::graph);
        node.setProperty (Tags::name, "Graph " + String(session->getNumGraphs() + 1));
        setRootNode (node);
        addMissingIONodes();
        session->addGraph (node, true);
        findSibling<GuiController>()->stabilizeContent();
    }
    else
    {
        AlertWindow::showMessageBoxAsync (
            AlertWindow::InfoIcon, "Elements", "Could not add new graph to session.");
    }
}

void EngineController::duplicateGraph()
{
    auto& world  = (dynamic_cast<AppController*>(getRoot()))->getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    ScopedPointer<RootGraph> newGraph = new RootGraph();
    if (engine->addGraph (newGraph.get()))
    {
        newGraph.release();
        root->savePluginStates();
        
        Node node (session->getCurrentGraph().getValueTree().createCopy());
        node.setProperty (Tags::name, node.getName().replace("(copy)","").trim() + String(" (copy)"));
        setRootNode (node);
        session->addGraph (node, true);
        findSibling<GuiController>()->stabilizeContent();
    }
    else
    {
        AlertWindow::showMessageBoxAsync (
            AlertWindow::InfoIcon, "Elements", "Could not duplicate graph.");
    }
}

void EngineController::removeGraph (int index)
{
    auto& world  = (dynamic_cast<AppController*>(getRoot()))->getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    Node node;
    if (isPositiveAndBelow (index, session->getNumGraphs()))
        node = session->getGraph (index);
    else
        node = session->getCurrentGraph();
    
    ValueTree graphs = session->getValueTree().getChildWithName (Tags::graphs);
    index = graphs.indexOf (node.getValueTree());
    graphs.removeChild (index, nullptr);
    index = jmin (index, graphs.getNumChildren() - 1);
    graphs.setProperty (Tags::active, index, nullptr);
    node = session->getCurrentGraph();
    if (node.isValid())
        setRootNode (node);
    else if (session->getNumGraphs() <= 0)
        root->clear();
    findSibling<GuiController>()->stabilizeContent();
}

void EngineController::connectChannels (const uint32 s, const int sc, const uint32 d, const int dc)
{
    jassert (root);
    auto src = root->getNodeForId (s);
    auto dst = root->getNodeForId (d);
    if (!src || !dst)
        return;
    addConnection (src->nodeId, src->getPortForChannel (PortType::Audio, sc, false),
                   dst->nodeId, dst->getPortForChannel (PortType::Audio, sc, true));
}

void EngineController::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    if (! root)
        return;
    root->removeConnection (s, sp, d, dp);
}

void EngineController::addNode (const Node& node)
{
    if (! root)
        return;
    
    if (KV_INVALID_NODE == root->addNode (node))
    {
        AlertWindow::showMessageBox (AlertWindow::InfoIcon,
            "Duplicate Node", String("Could not duplicate node: ") + node.getName());
    }
}

void EngineController::addPlugin (const PluginDescription& d)
{
    if (! root)
        return;
    root->addFilter (&d);
}

void EngineController::removeNode (const uint32 nodeId)
{
    jassert (root);
    if (! root)
        return;
    PluginWindow::closeCurrentlyOpenWindowsFor (nodeId);
    root->removeFilter (nodeId);
}

void EngineController::disconnectNode (const Node& node)
{
    if (! root)
        return;
    root->disconnectFilter (node.getNodeId());
}

void EngineController::activate()
{
    if (root != nullptr)
        return;
    
    Controller::activate();
    
    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& globals (app->getWorld());
    auto& devices (globals.getDeviceManager());
    auto engine (globals.getAudioEngine());
    auto session (globals.getSession());
    engine->setSession (session);
    RootGraph& graph (engine->getRootGraph());
    
    if (auto* device = devices.getCurrentAudioDevice())
        graph.setPlayConfigFor (device);
    
    root = new RootGraphController (engine->getRootGraph(), globals.getPluginManager());
    
    if (session->getNumGraphs() > 0)
        setRootNode (session->getGraph (0));
    
    engine->activate();
    devices.addChangeListener (this);
}

void EngineController::deactivate()
{
    Controller::deactivate();
    auto* app =   dynamic_cast<AppController*> (getRoot());
    auto& globals (app->getWorld());
    auto& devices (globals.getDeviceManager());
    auto engine   (globals.getAudioEngine());
    devices.removeChangeListener (this);
    if (root)
    {
        root->removeChangeListener (this);
        root->savePluginStates();
        root = nullptr;
    }
}

void EngineController::clear()
{
    if (root)
        root->clear();
}

void EngineController::setRootNode (const Node& newRootNode)
{
    if (! newRootNode.hasNodeType (Tags::graph))
    {
        jassertfalse; // needs to be a graph
        return;
    }

    DBG("[EL] updating engine/session in set Root Node: FIXME");
    auto engine = (dynamic_cast<AppController*> (getRoot()))->getGlobals().getAudioEngine();
    auto session = (dynamic_cast<AppController*> (getRoot()))->getGlobals().getSession();
    engine->setSession (session);
    
    DBG("[EL] setting root node: " << newRootNode.getName());
    root->setNodeModel (newRootNode);
    ValueTree nodes = newRootNode.getNodesValueTree();
    for (int i = nodes.getNumChildren(); --i >= 0;)
    {
        Node model (nodes.getChild(i), false);
        GraphNodePtr node = model.getGraphNode();
        if (node && node->isAudioIONode())
            model.resetPorts();
    }
}

void EngineController::addMissingIONodes()
{
    GraphNodePtr ioNodes [IOProcessor::numDeviceTypes];
    for (int i = 0; i < root->getNumFilters(); ++i)
    {
        GraphNodePtr node = root->getNode (i);
        if (node->isMidiIONode() || node->isAudioIONode())
        {
            auto* proc = dynamic_cast<IOProcessor*> (node->getAudioProcessor());
            ioNodes [proc->getType()] = node;
        }
    }
    
    for (int t = 0; t < IOProcessor::numDeviceTypes; ++t)
    {
        if (nullptr != ioNodes [t])
            continue;
        
        PluginDescription desc;
        desc.pluginFormatName = "Internal";
        
        switch (t)
        {
            case IOProcessor::audioInputNode:
                desc.fileOrIdentifier = "audio.input";
                break;
            case IOProcessor::audioOutputNode:
                desc.fileOrIdentifier = "audio.output";
                break;
            case IOProcessor::midiInputNode:
                desc.fileOrIdentifier = "midi.input";
                break;
            case IOProcessor::midiOutputNode:
                desc.fileOrIdentifier = "midi.output";
                break;
        }
        
        auto nodeId = root->addFilter (&desc);
        ioNodes[t] = root->getNodeForId (nodeId);
        jassert(ioNodes[t] != nullptr);
    }
}

void EngineController::changeListenerCallback (ChangeBroadcaster* cb)
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& devices (app->getWorld().getDeviceManager());
    auto& processor (root->getRootGraph());
    
    if (cb == (ChangeBroadcaster*) &devices)
    {
        if (auto* device = devices.getCurrentAudioDevice())
        {
            auto session = app->getWorld().getSession();
            auto nodes = session->getActiveGraph().getValueTree().getChildWithName(Tags::nodes);
            processor.suspendProcessing (true);
            processor.setPlayConfigFor (device);
            
            for (int i = nodes.getNumChildren(); --i >= 0;)
            {
                Node model (nodes.getChild(i), false);
                GraphNodePtr node = model.getGraphNode();
                if (node && node->isAudioIONode())
                {
                    (dynamic_cast<IOP*>(node->getProcessor()))->releaseResources();
                    (dynamic_cast<IOP*>(node->getProcessor()))->setParentGraph (&processor);
                    (dynamic_cast<IOP*>(node->getProcessor()))->prepareToPlay (device->getCurrentSampleRate(),
                                                                               device->getCurrentBufferSizeSamples());

                    model.resetPorts();
                }
            }
            
            root->syncArcsModel();
            processor.suspendProcessing (false);
        }
    }
}

}
