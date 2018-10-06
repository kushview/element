    

#include "session/Session.h"
#include "controllers/DevicesController.h"
#include "Globals.h"

namespace Element {

class DevicesController::Impl
{
public:
    Impl (DevicesController& o) : owner(o) { }
    ~Impl() { }

private:
    DevicesController& owner;
};

DevicesController::DevicesController()
{
    impl.reset (new Impl (*this));
}

DevicesController::~DevicesController()
{
    impl.reset (nullptr);
}

void DevicesController::activate()
{
    Controller::activate();
}

void DevicesController::deactivate()
{
    Controller::deactivate();
}

void DevicesController::add (const ControllerDevice& device)
{
    auto session = getWorld().getSession();
    if (! session) return;
    auto controllers = session->getValueTree().getChildWithName(Tags::controllers);
    if (controllers.indexOf (device.getValueTree()) < 0)
        controllers.addChild (device.getValueTree(), -1, nullptr);
}

void DevicesController::remove (const ControllerDevice& device)
{
    if (auto session = getWorld().getSession())
        session->getValueTree().getChildWithName(Tags::controllers)
            .removeChild (device.getValueTree(), nullptr);
}

}
