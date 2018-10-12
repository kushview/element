
#include "controllers/MappingController.h"
#include "engine/MappingEngine.h"
#include "session/ControllerDevice.h"
#include "Globals.h"

namespace Element {

class MappingController::Impl 
{
public:
    Impl() { }
    ~Impl() { }
};

MappingController::MappingController()
{
    impl.reset (new Impl());
}

MappingController::~MappingController()
{
    impl = nullptr;
}

void MappingController::activate()
{
    Controller::activate();
    capturedConnection = getWorld().getMappingEngine().capturedSignal().connect (
        std::bind (&MappingController::onEventCaptured, this));
    getWorld().getMappingEngine().startMapping();
    getWorld().getMappingEngine().capture (true);
}

void MappingController::deactivate() 
{
    Controller::deactivate();
    capturedConnection.disconnect();
}

void MappingController::onEventCaptured()
{
    auto& mapping (getWorld().getMappingEngine());
    DBG("received captured event");
    traceMidi (mapping.getCapturedMidiMessage());
    DBG(mapping.getCapturedControl().getValueTree().toXmlString());
}

}