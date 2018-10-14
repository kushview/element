
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
    getWorld().getMappingEngine().startMapping();
}

void DevicesController::deactivate()
{
    Controller::deactivate();
    getWorld().getMappingEngine().startMapping();
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
        refresh (device);
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
        refresh (device);
    }
    else
    {
        DBG(String("[EL] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

void DevicesController::refresh (const ControllerDevice& device)
{
    getWorld().getMappingEngine().refreshInput (device);
}

void DevicesController::refresh()
{
    auto& mapping (getWorld().getMappingEngine());
    auto session = getWorld().getSession();
    mapping.clear();
    for (int i = 0; i < session->getNumControllerDevices(); ++i)
        mapping.addInput (session->getControllerDevice (i));

    for (int i = 0; i < session->getNumControllerMaps(); ++i)
    {
        const auto child (session->getControllerMap (i));
        const int parameter = child.getParameterIndex();
        Node node = session->findNodeById (Uuid (child.getProperty (Tags::node).toString()));
        ControllerDevice device = session->findControllerDeviceById (
            Uuid (child.getProperty (Tags::controller).toString()));
        ControllerDevice::Control control = device.findControlById (
            Uuid (child.getProperty (Tags::control).toString()));
        const bool valid = device.isValid() && control.isValid() && node.isValid() && parameter >= 0;
        if (valid && mapping.addHandler (control, node, parameter))
        {
            DBG("[EL] added handler in refresh: " << control.getName().toString());
        }
        else
        {
            DBG("[EL] failed adding handler: " << control.getName().toString());
        }
    }

    mapping.startMapping();
}

}
