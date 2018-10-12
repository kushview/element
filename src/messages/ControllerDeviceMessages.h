
#pragma once

#include "session/ControllerDevice.h"

namespace Element {

struct RefreshControllerDeviceMessage : public AppMessage
{
    RefreshControllerDeviceMessage (const ControllerDevice& d)
        : device (d) { }
    ~RefreshControllerDeviceMessage() { }
    const ControllerDevice device;
};

struct AddControllerDeviceMessage : public AppMessage
{
    AddControllerDeviceMessage (const ControllerDevice& d)
        : device (d) { }
    ~AddControllerDeviceMessage() noexcept { }
    const ControllerDevice device;
};

struct RemoveControllerDeviceMessage : public AppMessage
{
    RemoveControllerDeviceMessage (const ControllerDevice& d)
        : device (d) { }
    ~RemoveControllerDeviceMessage() noexcept { }
    const ControllerDevice device;
};

struct AddControlMessage : public AppMessage
{
    AddControlMessage (const ControllerDevice& d, const ControllerDevice::Control& c)
        : device (d), control (c) { }
    ~AddControlMessage() noexcept { }
    const ControllerDevice device;
    const ControllerDevice::Control control;
};

struct RemoveControlMessage : public AppMessage
{
    RemoveControlMessage (const ControllerDevice& d, const ControllerDevice::Control& c)
        : device (d), control (c) { }
    ~RemoveControlMessage() noexcept { }
    const ControllerDevice device;
    const ControllerDevice::Control control;
};

}
