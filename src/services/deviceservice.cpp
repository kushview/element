// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services/deviceservice.hpp"

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
    // Broadcast MIDI device hot-plug/removal to interested services (the callback
    // is delivered on the message thread).
    deviceListConnection = juce::MidiDeviceListConnection::make (
        [this] { sigMidiDevicesChanged(); });
}

void DeviceService::deactivate()
{
    deviceListConnection.reset();
    Service::deactivate();
}

} // namespace element
