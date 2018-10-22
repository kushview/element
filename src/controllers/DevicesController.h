#pragma once

#include "controllers/AppController.h"
#include "session/ControllerDevice.h"

namespace Element {

class DevicesController : public AppController::Child
{
public:
    DevicesController();
    ~DevicesController();

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
    class Impl; friend class Impl;
    std::unique_ptr<Impl> impl;
};

}
