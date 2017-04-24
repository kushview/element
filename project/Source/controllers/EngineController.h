#include "controllers/Controller.h"

namespace Element {
    
class GraphController;
    
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
    
    void addConnection (const uint32, const uint32, const uint32, const uint32);
    void connectChannels (const uint32, const int, const uint32, const int);
    void removeConnection (const uint32, const uint32, const uint32, const uint32);
    
private:
    ScopedPointer<GraphController> root;
    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
};
    
}
