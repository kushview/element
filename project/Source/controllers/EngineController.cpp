
#include "ElementApp.h"
#include "controllers/AppController.h"
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
    jassert (root);
    root->removeConnection (s, sp, d, dp);
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
    RootGraph& graph (engine->getRootGraph());
    
    if (auto* device = devices.getCurrentAudioDevice())
        graph.setPlayConfigFor (device);
    
    root = new RootGraphController (engine->getRootGraph(), globals.getPluginManager());
    
    if (session->getNumGraphs() > 0)
        setRootNode (Node (session->getGraphValueTree (0)));
    
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

    root->setNodeModel (newRootNode);
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
            auto nodes = session->getGraphValueTree(0).getChildWithName(Tags::nodes);
            processor.suspendProcessing (true);
            processor.setPlayConfigFor (device);
            for (int i = nodes.getNumChildren(); --i >= 0;)
            {
                const Node model (nodes.getChild(i), false);
                GraphNodePtr node = model.getGraphNode();
                if (node && node->isAudioIONode())
                {
                    (dynamic_cast<IOP*>(node->getProcessor()))->releaseResources();
                    (dynamic_cast<IOP*>(node->getProcessor()))->setParentGraph (&processor);
                    (dynamic_cast<IOP*>(node->getProcessor()))->prepareToPlay (device->getCurrentSampleRate(),
                                                                               device->getCurrentBufferSizeSamples());
                    processor.removeIllegalConnections();
                    node->resetPorts();
                    auto newPorts = node->getMetadata().getChildWithName(Tags::ports).createCopy();
                    auto data = model.getValueTree();
                    data.removeChild(model.getPortsValueTree(), nullptr);
                    data.addChild(newPorts, -1, nullptr);
                }
            }
            
            processor.suspendProcessing (false);
        }
    }
}

}
