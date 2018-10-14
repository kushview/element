
#include "controllers/AppController.h"
#include "engine/GraphNode.h"
#include "session/ControllerDevice.h"
#include "Signals.h"

namespace Element {

class MappingController : public AppController::Child
{
public:
    MappingController();
    ~MappingController();

    void activate() override;
    void deactivate() override;    
    void learn (const bool shouldLearn = true);
    bool isLearning() const;
    void remove (const ControllerMap&);
private:
    class Impl; friend class Impl;
    std::unique_ptr<Impl> impl;
    boost::signals2::connection capturedConnection;
    boost::signals2::connection capturedParamConnection;
    void onControlCaptured();
    void onParameterCaptured (const Node&, int);
};

}
