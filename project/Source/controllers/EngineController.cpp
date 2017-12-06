
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
#include "session/DeviceManager.h"
#include "session/PluginManager.h"
#include "session/Node.h"
#include "Globals.h"
#include "Settings.h"

#include "controllers/EngineController.h"

namespace Element {

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
    if (root)
        root->addConnection (s, sp, d, dp);
}

void EngineController::addGraph()
{
    auto& world  = getWorld();
    auto engine  = world.getAudioEngine();
    auto session = world.getSession();
    
    ScopedPointer<RootGraph> newGraph = new RootGraph();
    if (engine->addGraph (newGraph.get()))
    {
        Node node (Tags::graph);
        node.setProperty (Tags::name, "Graph " + String(session->getNumGraphs() + 1));
        session->addGraph (node, true);
        setRootNode (node);
        addMissingIONodes();
        newGraph.release();
    }
    else
    {
        AlertWindow::showMessageBoxAsync (
            AlertWindow::InfoIcon, "Elements", "Could not add new graph to session.");
    }
    
    findSibling<GuiController>()->stabilizeContent();
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
        Node node (session->getCurrentGraph().getValueTree().createCopy());
        node.setProperty (Tags::name, node.getName().replace("(copy)","").trim() + String(" (copy)"));
        session->addGraph (node, true);
        setRootNode (node);
    }
    else
    {
        AlertWindow::showMessageBoxAsync (
            AlertWindow::InfoIcon, "Elements", "Could not duplicate graph.");
    }
    
    findSibling<GuiController>()->stabilizeContent();
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
    
    if (auto* g = engine->getGraph (index))
    {
        jassert (g == &root->getGraph());
        root = nullptr;
        engine->removeGraph (g);
    }
    
    index = jmin (index, graphs.getNumChildren() - 1);
    graphs.setProperty (Tags::active, index, nullptr);
    node = session->getCurrentGraph();
    if (node.isValid())
        setRootNode (node);
    else if (root && session->getNumGraphs() <= 0)
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
                   dst->nodeId, dst->getPortForChannel (PortType::Audio, dc, true));
}

void EngineController::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    if (! root)
        return;
    root->removeConnection (s, sp, d, dp);
}

void EngineController::addNode (const Node& node)
{
    if (root && KV_INVALID_NODE == root->addNode (node))
    {
        AlertWindow::showMessageBox (AlertWindow::InfoIcon,
            "Duplicate Node", String("Could not duplicate node: ") + node.getName());
    }
}

void EngineController::addPlugin (const PluginDescription& desc, const bool verified, const float rx, const float ry)
{
    if (! root)
        return;
    
    OwnedArray<PluginDescription> plugs;
    if (! verified)
    {
        auto* format = getWorld().getPluginManager().getAudioPluginFormat (desc.pluginFormatName);
        jassert(format != nullptr);
        auto& list (getWorld().getPluginManager().availablePlugins());
        if (list.scanAndAddFile (desc.fileOrIdentifier, false, plugs, *format)) {
            getWorld().getPluginManager().saveUserPlugins (getWorld().getSettings());
        }
    }
    else
    {
        plugs.add (new PluginDescription (desc));
    }
    
    if (plugs.size() > 0)
        root->addFilter (plugs.getFirst(), rx, ry);
    else
        AlertWindow::showMessageBoxAsync (AlertWindow::NoIcon, "Add Plugin", String("Could not add ") + desc.name + " for an unknown reason");
}

void EngineController::removeNode (const uint32 nodeId)
{
    if (! root)
        return;
    if (auto* gui = findSibling<GuiController>())
        gui->closePluginWindowsFor (nodeId, true);
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

    if (session->getNumGraphs() > 0)
    {
        DBG ("[EL] loading current graph in engine activate");
        setRootNode (session->getCurrentGraph());
    }
    
    engine->activate();
    devices.addChangeListener (this);
}

void EngineController::deactivate()
{
    Controller::deactivate();
    auto& globals (getWorld());
    auto& devices (globals.getDeviceManager());
    auto engine   (globals.getAudioEngine());
    engine->setSession (nullptr);
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
    if (! newRootNode.isRootGraph())
    {
        jassertfalse; // needs to be a graph
        return;
    }

    auto engine   = getWorld().getAudioEngine();
    auto session  = getWorld().getSession();
    auto& devices = getWorld().getDeviceManager();
    
    const int index = session->getValueTree().getChildWithName(Tags::graphs).indexOf (newRootNode.getValueTree ());
    
    /* Unload the existing graph if necessary */
    if (root)
    {
        if (auto* gui = findSibling<GuiController>())
            gui->closeAllPluginWindows();
        root->savePluginStates();
        root->unloadGraph();
    }
    
    /* Ensure enough processors allocated for number of graphs on the session */
    for (int i = 0; i < session->getNumGraphs(); ++i)
    {
        if (nullptr != engine->getGraph (i))
            continue;
        engine->addGraph (new RootGraph());
    }

    if (auto* proc = engine->getGraph (index))
    {
        proc->setMidiChannel ((int) newRootNode.getProperty (Tags::midiChannel, 0));
        root = new RootGraphController (*proc, getWorld().getPluginManager());
    }
    else
    {
        DBG("[EL] couldn't find graph processor for node.");
        root = nullptr;
    }
    
    if (root)
    {
        DBG("[EL] setting root: " << newRootNode.getName());
        root->setNodeModel (newRootNode);
        if (auto* device = devices.getCurrentAudioDevice())
            root->getRootGraph().setPlayConfigFor (device);
        
        ValueTree nodes = newRootNode.getNodesValueTree();
        for (int i = nodes.getNumChildren(); --i >= 0;)
        {
            Node model (nodes.getChild(i), false);
            GraphNodePtr node = model.getGraphNode();
            if (node && node->isAudioIONode())
                model.resetPorts();
        }
        
        engine->setCurrentGraph (index);
        
        if (auto* gui = findSibling<GuiController>())
            gui->showPluginWindowsFor (newRootNode);
    }
    else
    {
        DBG("[EL] no graph controller for node: " << newRootNode.getName());
    }
    
    engine->refreshSession();
}
void EngineController::updateRootGraphMidiChannel (const int index, const int midiChannel)
{
    auto engine   = getWorld().getAudioEngine();
    if (auto* g = engine->getGraph (index)) {
        g->setMidiChannel (midiChannel);
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
}

void EngineController::changeListenerCallback (ChangeBroadcaster* cb)
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& devices (app->getWorld().getDeviceManager());
   
    
    if (cb == (ChangeBroadcaster*) &devices && root != nullptr)
    {
        auto& processor (root->getRootGraph());
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
