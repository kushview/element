#include "controllers/Controller.h"

namespace Element {
    
class RootGraphController;
class Node;
    
class EngineController : public Controller,
                         private ChangeListener
{
public:
    EngineController();
    ~EngineController();
    
    void activate() override;
    void deactivate() override;
    
    /** Adds a new node to the current graph. */
    void addNode (const Node& node);
    
    /** Adds a plugin by description to the current graph */
    void addPlugin (const PluginDescription&);
    
    /** Removes a node from the current graph */
    void removeNode (const uint32);
    
    void addGraph();
    void removeGraph (int index = -1);
    void duplicateGraph();
    void addSubGraph();
    
    void addConnection (const uint32, const uint32, const uint32, const uint32);
    void connectChannels (const uint32, const int, const uint32, const int);
    void removeConnection (const uint32, const uint32, const uint32, const uint32);
    void disconnectNode (const Node& node);
    
    void clear();
    void setRootNode (const Node&);
    
private:
    ScopedPointer<RootGraphController> root;
    OwnedArray<RootGraphController> rootGraphs;
    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void addMissingIONodes();
};
    
}
