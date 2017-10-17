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
    
    void addPlugin (const PluginDescription&);
    void removeNode (const uint32);
    
    void addGraph();
    void removeGraph (int index = -1);
    void addSubGraph();
    
    void addConnection (const uint32, const uint32, const uint32, const uint32);
    void connectChannels (const uint32, const int, const uint32, const int);
    void removeConnection (const uint32, const uint32, const uint32, const uint32);
    
    void clear();
    void setRootNode (const Node&);
    
private:
    ScopedPointer<RootGraphController> root;
    OwnedArray<RootGraphController> rootGraphs;
    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
};
    
}
