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

#include "services.hpp"
#include <element/controllerdevice.hpp>

namespace element {

class DeviceService : public Service
{
public:
    DeviceService();
    ~DeviceService();

    void activate() override;
    void deactivate() override;

    void add (const ControllerDevice&);
    void add (const ControllerDevice&, const ControllerDevice::Control&);
    void add (const File& file);
    void remove (const ControllerDevice&);
    void remove (const ControllerDevice&, const ControllerDevice::Control&);
    void refresh (const ControllerDevice&);
    void refresh();

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace element
