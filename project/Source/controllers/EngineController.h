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
    
private:
    ScopedPointer<GraphController> root;
    friend class ChangeBroadcaster;
    void changeListenerCallback (ChangeBroadcaster*) override;
};

}
