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

#include "controllers/DevicesController.h"
#include "engine/MappingEngine.h"
#include "engine/MidiEngine.h"
#include "session/session.hpp"
#include "globals.hpp"

namespace Element {

class DevicesController::Impl
{
public:
    Impl (DevicesController& o) : owner (o) {}
    ~Impl() {}

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
    auto& midi (getWorld().getMidiEngine());
    if (! mapping.addInput (device, midi))
        return;

    auto session = getWorld().getSession();
    if (! session)
        return;
    auto controllers = session->getValueTree().getChildWithName (Tags::controllers);
    if (controllers.indexOf (device.getValueTree()) < 0)
    {
        controllers.addChild (device.getValueTree(), -1, nullptr);
        refresh (device);
    }
}

void DevicesController::add (const ControllerDevice& device, const ControllerDevice::Control& control)
{
    auto session = getWorld().getSession();
    if (session && session->indexOf (device) >= 0 && device.indexOf (control) < 0)
    {
        auto data = device.getValueTree();
        data.addChild (control.getValueTree(), -1, nullptr);
        refresh (device);
    }
    else
    {
        DBG (String ("[EL] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

void DevicesController::add (const File& file)
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

        if (auto s = getWorld().getSession())
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

void DevicesController::remove (const ControllerDevice& device)
{
    auto& mapping (getWorld().getMappingEngine());
    if (! mapping.removeInput (device))
        return;
    if (auto session = getWorld().getSession())
        session->getValueTree().getChildWithName (Tags::controllers).removeChild (device.getValueTree(), nullptr);
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
        DBG (String ("[EL] device not found in session: ") << session->getName() << " / " << device.getName().toString());
    }
}

void DevicesController::refresh (const ControllerDevice& device)
{
    refresh();
    return;
#if 0
    // TODO: handle individual re-build of device handlers and state
    // Saved for reference.
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
#endif
}

void DevicesController::refresh()
{
    auto& mapping (getWorld().getMappingEngine());
    auto& midi (getWorld().getMidiEngine());
    auto session = getWorld().getSession();
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
            DBG ("[EL] added handler in refresh: " << control.getName().toString());
        }
        else
        {
            DBG ("[EL] failed adding handler: " << control.getName().toString());
        }
    }

    mapping.startMapping();
}

} // namespace Element
