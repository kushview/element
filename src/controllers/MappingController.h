
#include "controllers/AppController.h"
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
    
private:
    class Impl; friend class Impl;
    std::unique_ptr<Impl> impl;
    boost::signals2::connection capturedConnection;
    void onEventCaptured();
};

}
