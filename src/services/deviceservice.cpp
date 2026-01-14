// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services/deviceservice.hpp"
#include "engine/mappingengine.hpp"
#include "engine/midiengine.hpp"
#include <element/session.hpp>
#include <element/context.hpp>

namespace element {

class DeviceService::Impl
{
public:
    Impl (DeviceService& o) : owner (o) {}
    ~Impl() {}

private:
    [[maybe_unused]] DeviceService& owner;
};

DeviceService::DeviceService()
{
    impl.reset (new Impl (*this));
}

DeviceService::~DeviceService()
{
    impl.reset (nullptr);
}

void DeviceService::activate()
{
    Service::activate();
}

void DeviceService::deactivate()
{
    Service::deactivate();
}

void DeviceService::add (const Controller& device)
{
    auto& mapping (context().mapping());
    auto& midi (context().midi());
    if (! mapping.addInput (device, midi))
        return;

    auto session = context().session();
    if (! session)
        return;
    auto controllers = session->data().getChildWithName (tags::controllers);
    if (controllers.indexOf (device.data()) < 0)
    {
        controllers.addChild (device.data(), -1, nullptr);
        refresh (device);
    }
}

void DeviceService::add (const Controller& device, const Control& control)
{
    auto session = context().session();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) < 0)
    {
        auto data = device.data();
        data.addChild (control.data(), -1, nullptr);
        refresh (device);
    }
    else
    {
        DBG (String ("[element] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

void DeviceService::add (const File& file)
{
    ValueTree data;
    if (auto xml = XmlDocument::parse (file))
        data = ValueTree::fromXml (*xml);

    if (data.isValid() && data.hasType (types::Controller))
    {
        // Avoid UUID conflicts by replacing all
        data.setProperty (tags::uuid, Uuid().toString(), 0);
        for (int i = 0; i < data.getNumChildren(); ++i)
            data.getChild (i).setProperty (tags::uuid, Uuid().toString(), 0);

        if (auto s = context().session())
        {
            s->data().getChildWithName (tags::controllers).addChild (data, -1, 0);
            refresh();
        }
    }
    else
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Open Controller Device", "Could not open the controller device file.");
    }
}

void DeviceService::remove (const Controller& device)
{
    auto& mapping (context().mapping());
    if (! mapping.removeInput (device))
        return;
    if (auto session = context().session())
        session->data().getChildWithName (tags::controllers).removeChild (device.data(), nullptr);
}

void DeviceService::remove (const Controller& device, const Control& control)
{
    auto session = context().session();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) >= 0)
    {
        auto data = device.data();
        data.removeChild (control.data(), nullptr);
        refresh (device);
    }
    else
    {
        DBG (String ("[element] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

void DeviceService::refresh (const Controller& device)
{
    refresh();
    return;
#if 0
    // TODO: handle individual re-build of device handlers and state
    // Saved for reference.
    auto session = context().session();
    auto& mapping = context().mapping();
    mapping.refreshInput (device);
    
    for (int i = 0; i < session->getNumControllerMaps(); ++i)
    {
        const auto child (session->getControllerMap (i));
        const int parameter = child.getParameterIndex();
        const ControllerMapObjects objects (session, child);

        if (objects.isValid() && mapping.addHandler (objects.control, objects.node, parameter))
        {
            DBG("[element] added handler in refresh: " << objects.control.getName().toString());
        }
    }
#endif
}

void DeviceService::refresh()
{
    auto& mapping (context().mapping());
    auto& midi (context().midi());
    auto session = context().session();
    mapping.clear();

    for (int i = 0; i < session->getNumControllers(); ++i)
        mapping.addInput (session->getController (i), midi);

    for (int i = 0; i < session->getNumControllerMaps(); ++i)
    {
        const auto child (session->getControllerMap (i));
        const int parameter = child.getParameterIndex();
        Node node = session->findNodeById (Uuid (child.getProperty (tags::node).toString()));
        Controller device = session->findControllerById (
            Uuid (child.getProperty (types::Controller).toString()));
        Control control = device.findControlById (
            Uuid (child.getProperty (tags::control).toString()));

        if (mapping.addHandler (control, node, parameter))
        {
            DBG ("[element] added handler in refresh: " << control.getName().toString());
        }
        else
        {
            DBG ("[element] failed adding handler: " << control.getName().toString());
        }
    }

    mapping.startMapping();
}

} // namespace element
