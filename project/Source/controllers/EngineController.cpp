
#include "ElementApp.h"
#include "controllers/AppController.h"
#include "controllers/GraphController.h"
#include "Globals.h"

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

    auto* app = dynamic_cast<AppController*> (getRoot());
    auto& g (app->getWorld());
    AudioEnginePtr engine (g.engine());
    root = new GraphController (engine->graph(), g.plugins());
    root->addChangeListener (this);
}

void EngineController::changeListenerCallback (ChangeBroadcaster*)
{
    if (auto* app = dynamic_cast<AppController*> (getRoot())) {
        DBG("GRAPH CHANGED");
        GraphLog::print (*root);
        app->postMessage (new Message());
    }
    
}
    
}
