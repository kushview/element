
#include "controllers/DevicesController.h"
#include "engine/MappingEngine.h"
#include "session/Session.h"
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
    auto& mapping (getWorld().getMappingEngine());
    if (! mapping.addInput (device))
        return;

    auto session = getWorld().getSession();
    if (! session) return;
    auto controllers = session->getValueTree().getChildWithName (Tags::controllers);
    if (controllers.indexOf (device.getValueTree()) < 0)
        controllers.addChild (device.getValueTree(), -1, nullptr);
}

void DevicesController::add (const ControllerDevice& device, const ControllerDevice::Control& control)
{
    auto& mapping (getWorld().getMappingEngine());
    auto session = getWorld().getSession();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) < 0)
    {
        auto data = device.getValueTree();
        data.addChild (control.getValueTree(), -1, nullptr);
    }
    else
    {
        DBG(String("[EL] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }

    
}

void DevicesController::remove (const ControllerDevice& device)
{
    auto& mapping (getWorld().getMappingEngine());
    if (! mapping.removeInput (device))
        return;
    if (auto session = getWorld().getSession())
        session->getValueTree().getChildWithName (Tags::controllers)
            .removeChild (device.getValueTree(), nullptr);
}

void DevicesController::remove (const ControllerDevice& device, const ControllerDevice::Control& control)
{
    auto session = getWorld().getSession();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) >= 0)
    {
        auto data = device.getValueTree();
        data.removeChild (control.getValueTree(), nullptr);
    }
    else
    {
        DBG(String("[EL] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

}
