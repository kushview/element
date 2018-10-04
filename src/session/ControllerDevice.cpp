/*
    ControllerDevice.cpp - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#include "session/ControllerDevice.h"

namespace Element {

ControllerDevice::ControllerDevice()
    : ObjectModel (Identifier ("controllerDevice"))
{
    setMissingProperties();
}

ControllerDevice::ControllerDevice (const ValueTree& data)
    : ObjectModel (data)
{
    jassert(data.getType() == Identifier("controllerDevice"));
}

void ControllerDevice::setMissingProperties()
{
    stabilizePropertyString (Tags::name, "New Device");
}
}
