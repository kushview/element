#pragma once

#include "controllers/AppController.h"

namespace Element {

class ControllerDevice;

class DevicesController : public AppController::Child
{
public:
    DevicesController();
    ~DevicesController();
    
    void activate() override;
    void deactivate() override;

    void add (const ControllerDevice&);
    void remove (const ControllerDevice&);

private:
    class Impl; friend class Impl;
    std::unique_ptr<Impl> impl;
};

}
