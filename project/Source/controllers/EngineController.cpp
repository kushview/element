
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

namespace GraphLog {
    void print (GraphController& g)
    {
        DBG("[EL] graph: " << g.getGraph().getName());
        for (int i = 0; i < g.getNumFilters(); ++i)
        {
            PluginDescription d;
            g.getNode(i)->getPluginDescription(d);
            
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
    root->getGraph().connectChannels(PortType::Audio, s, sc, d, dc);
}

void EngineController::removeConnection (const uint32 s, const uint32 sp, const uint32 d, const uint32 dp)
{
    jassert(root);
    root->getGraph().removeConnection (s, sp, d, dp);
    root->sendChangeMessage();
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
    if (! root) return;
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
    auto& settings (globals.getSettings());
    auto& devices (globals.getDeviceManager());
    
    AudioEnginePtr engine (globals.getAudioEngine());
    GraphProcessor& graph (engine->graph());
    
    if (auto* device = devices.getCurrentAudioDevice())
    {
        ignoreUnused (device);
        
        const int numIns = device->getActiveOutputChannels().countNumberOfSetBits();
        const int numOuts = device->getActiveInputChannels().countNumberOfSetBits();
        const int bufferSize = device->getCurrentBufferSizeSamples();
        const double sampleRate = device->getCurrentSampleRate();
        
        graph.setPlayConfigDetails (numIns, numOuts, sampleRate, bufferSize);
    }
    
    root = new GraphController (engine->graph(), globals.getPluginManager());
    
    if (ScopedXml xml = settings.getLastGraph())
    {
        const Node lastGraph (ValueTree::fromXml (*xml), false);
        setRootNode (lastGraph);
    }
    
    engine->activate();
    root->addChangeListener (this);
}

void EngineController::deactivate()
{
    Controller::deactivate();
}

void EngineController::clear()
{
    if (root)
        root->clear();
}

void EngineController::setRootNode (const Node& node)
{
    if (! node.hasNodeType (Tags::graph)) {
        jassertfalse; // needs to be a graph
        return;
    }
    
    root->clear();
    
    const ValueTree nodes (node.getNodesValueTree());
    const ValueTree arcs (node.getArcsValueTree());
    
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        const Node node (nodes.getChild(i), false);
        PluginDescription desc; node.getPluginDescription (desc);
        const uint32 nodeId = root->addFilter (&desc, 0.0, 0.0, node.getNodeId());
        jassert(nodeId == node.getNodeId());
        if (GraphNodePtr obj = root->getNodeForId (nodeId))
        {
            DBG("Added plugin: " << node.getName());
        }
    }
    
    jassert (nodes.getNumChildren() == root->getNumFilters());
    
    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        const ValueTree arc (arcs.getChild (i));
        addConnection ((uint32)(int) arc.getProperty (Tags::sourceNode),
                       (uint32)(int) arc.getProperty (Tags::sourcePort),
                       (uint32)(int) arc.getProperty (Tags::destNode),
                       (uint32)(int) arc.getProperty (Tags::destPort));
    }
    
    jassert (arcs.getNumChildren() == root->getNumConnections());
}

void EngineController::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* app = dynamic_cast<AppController*> (getRoot())) {
        ignoreUnused (app);
    }
}
}
