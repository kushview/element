
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
        root->removeChangeListener(this);
        root = nullptr;
    }
}

void EngineController::addPlugin (const PluginDescription& d)
{
    if (! root)
        return;
    root->addFilter (&d);
}
    
void EngineController::activate()
{
    if (root != nullptr)
        return;
    
    Controller::activate();
    
    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& g (app->getWorld());
    auto& s (g.getSettings());
    auto* props = s.getUserSettings();
    AudioEnginePtr engine (g.getAudioEngine());
    root = new GraphController (engine->graph(), g.getPluginManager());
    
    if (ScopedXml xml = props->getXmlValue ("lastGraph"))
    {
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
            const ValueTree arc (arcs.getChild(i));
            currentArcs.addChild (arc.createCopy(), -1, nullptr);
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
