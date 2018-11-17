
#include "controllers/DevicesController.h"
#include "engine/MappingEngine.h"
#include "session/Session.h"
#include "session/UnlockStatus.h"
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
    returnIfNotFullVersion
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
    returnIfNotFullVersion
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

void DevicesController::add (const File& file)
{
    ValueTree data;
    if (ScopedXml xml = XmlDocument::parse (file))
        data = ValueTree::fromXml (*xml);
    if (data.isValid() && data.hasType (Tags::controller))
    {
        // Avoid UUID conflicts by replacing all
        data.setProperty (Tags::uuid, Uuid().toString(), 0);
        for (int i = 0; i < data.getNumChildren(); ++i)
            data.getChild(i).setProperty (Tags::uuid, Uuid().toString(), 0);

        if (auto s = getWorld().getSession())
            s->getValueTree().getChildWithName (Tags::controllers)
                .addChild (data, -1, 0);
    }
    else
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Open Controller Device",
            "Could not open the controller device file.");
    }
}

void DevicesController::remove (const ControllerDevice& device)
{
    returnIfNotFullVersion
    auto& mapping (getWorld().getMappingEngine());
    if (! mapping.removeInput (device))
        return;
    if (auto session = getWorld().getSession())
        session->getValueTree().getChildWithName (Tags::controllers)
            .removeChild (device.getValueTree(), nullptr);
}

void DevicesController::remove (const ControllerDevice& device, const ControllerDevice::Control& control)
{
    returnIfNotFullVersion
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
    refresh();
    return;
    // TODO: handle individual re-build of device handlers and state
    auto session = getWorld().getSession();
    auto& mapping = getWorld().getMappingEngine();
    mapping.refreshInput (device);
    
    for (int i = 0; i < session->getNumControllerMaps(); ++i)
    {
        const auto child (session->getControllerMap (i));
        const int parameter = child.getParameterIndex();
        const ControllerMapObjects objects (session, child);

        if (objects.isValid() && mapping.addHandler (objects.control, objects.node, parameter))
        {
            DBG("[EL] added handler in refresh: " << objects.control.getName().toString());
        }
    }
}

void DevicesController::refresh()
{
    auto& mapping (getWorld().getMappingEngine());
    auto session = getWorld().getSession();
    mapping.clear();

    returnIfNotFullVersion

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
