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

#pragma once

#include "session/ControllerDevice.h"

namespace Element {

struct RefreshControllerDeviceMessage : public AppMessage
{
    RefreshControllerDeviceMessage (const ControllerDevice& d)
        : device (d) {}
    ~RefreshControllerDeviceMessage() {}
    const ControllerDevice device;
};

struct AddControllerDeviceMessage : public AppMessage
{
    AddControllerDeviceMessage (const ControllerDevice& d)
        : device (d) {}
    AddControllerDeviceMessage (const File& f)
        : file (f) {}
    ~AddControllerDeviceMessage() noexcept {}
    const ControllerDevice device;
    const File file;
};

struct RemoveControllerDeviceMessage : public AppMessage
{
    RemoveControllerDeviceMessage (const ControllerDevice& d)
        : device (d) {}
    ~RemoveControllerDeviceMessage() noexcept {}
    const ControllerDevice device;
};

struct AddControlMessage : public AppMessage
{
    AddControlMessage (const ControllerDevice& d, const ControllerDevice::Control& c)
        : device (d), control (c) {}
    ~AddControlMessage() noexcept {}
    const ControllerDevice device;
    const ControllerDevice::Control control;
};

struct RemoveControlMessage : public AppMessage
{
    RemoveControlMessage (const ControllerDevice& d, const ControllerDevice::Control& c)
        : device (d), control (c) {}
    ~RemoveControlMessage() noexcept {}
    const ControllerDevice device;
    const ControllerDevice::Control control;
};

struct RemoveControllerMapMessage : public AppMessage
{
    RemoveControllerMapMessage (const ControllerMap& mapp)
        : controllerMap (mapp) {}
    ~RemoveControllerMapMessage() noexcept {}
    const ControllerMap controllerMap;
};

} // namespace Element
