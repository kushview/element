// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services/deviceservice.hpp"
#include "engine/mappingengine.hpp"
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

void DeviceService::refresh()
{
    if (auto session = context().session())
        context().mapping().rebuildBindings (session);
}

} // namespace element
