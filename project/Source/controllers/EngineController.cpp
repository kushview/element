
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GraphController.h"
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
    jassert(root);
    if (! root)
        return;
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
    auto* props = settings.getUserSettings();
    AudioEnginePtr engine (globals.getAudioEngine());
    root = new GraphController (engine->graph(), globals.getPluginManager());
    
    if (ScopedXml xml = props->getXmlValue ("lastGraph"))
    {
        root->clear();
        
        const Node lastGraph (ValueTree::fromXml (*xml), false);
        const ValueTree nodes (lastGraph.getNodesValueTree());
        const ValueTree arcs (lastGraph.getArcsValueTree());

        ValueTree currentGraph = root->getGraph().getGraphModel();
        ValueTree currentNodes = root->getGraph().getNodesModel();
        ValueTree currentArcs = root->getGraph().getArcsModel();
        currentNodes.removeAllChildren (nullptr);
        currentArcs.removeAllChildren (nullptr);

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
            addConnection ((uint32)(int) arc.getProperty("sourceNode"),
                           (uint32)(int) arc.getProperty("sourcePort"),
                           (uint32)(int) arc.getProperty("destNode"),
                           (uint32)(int) arc.getProperty("destPort"));
        }
    }
    
    root->addChangeListener (this);
}

void EngineController::deactivate()
{
    Controller::deactivate();
}

void EngineController::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* app = dynamic_cast<AppController*> (getRoot())) {
        ignoreUnused(app);
    }
}
}
