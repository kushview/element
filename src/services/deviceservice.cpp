/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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
    DeviceService& owner;
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

void DeviceService::add (const ControllerDevice& device)
{
    auto& mapping (context().mapping());
    auto& midi (context().midi());
    if (! mapping.addInput (device, midi))
        return;

    auto session = context().session();
    if (! session)
        return;
    auto controllers = session->getValueTree().getChildWithName (Tags::controllers);
    if (controllers.indexOf (device.getValueTree()) < 0)
    {
        controllers.addChild (device.getValueTree(), -1, nullptr);
        refresh (device);
    }
}

void DeviceService::add (const ControllerDevice& device, const ControllerDevice::Control& control)
{
    auto session = context().session();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) < 0)
    {
        auto data = device.getValueTree();
        data.addChild (control.getValueTree(), -1, nullptr);
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

    if (data.isValid() && data.hasType (Tags::controller))
    {
        // Avoid UUID conflicts by replacing all
        data.setProperty (Tags::uuid, Uuid().toString(), 0);
        for (int i = 0; i < data.getNumChildren(); ++i)
            data.getChild (i).setProperty (Tags::uuid, Uuid().toString(), 0);

        if (auto s = context().session())
        {
            s->getValueTree().getChildWithName (Tags::controllers).addChild (data, -1, 0);
            refresh();
        }
    }
    else
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Open Controller Device", "Could not open the controller device file.");
    }
}

void DeviceService::remove (const ControllerDevice& device)
{
    auto& mapping (context().mapping());
    if (! mapping.removeInput (device))
        return;
    if (auto session = context().session())
        session->getValueTree().getChildWithName (Tags::controllers).removeChild (device.getValueTree(), nullptr);
}

void DeviceService::remove (const ControllerDevice& device, const ControllerDevice::Control& control)
{
    auto session = context().session();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) >= 0)
    {
        auto data = device.getValueTree();
        data.removeChild (control.getValueTree(), nullptr);
        refresh (device);
    }
    else
    {
        DBG (String ("[element] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

void DeviceService::refresh (const ControllerDevice& device)
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

    for (int i = 0; i < session->getNumControllerDevices(); ++i)
        mapping.addInput (session->getControllerDevice (i), midi);

    for (int i = 0; i < session->getNumControllerMaps(); ++i)
    {
        const auto child (session->getControllerMap (i));
        const int parameter = child.getParameterIndex();
        Node node = session->findNodeById (Uuid (child.getProperty (Tags::node).toString()));
        ControllerDevice device = session->findControllerDeviceById (
            Uuid (child.getProperty (Tags::controller).toString()));
        ControllerDevice::Control control = device.findControlById (
            Uuid (child.getProperty (Tags::control).toString()));

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
