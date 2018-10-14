/*
    ControllerDevice.cpp - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#include "session/ControllerDevice.h"

namespace Element {

ControllerDevice::ControllerDevice (const ValueTree& data)
    : ObjectModel (data)
{
    if (data.isValid())
    {
        jassert (data.hasType (Tags::controller));
        jassert (data.hasProperty (Tags::uuid));
        setMissingProperties();
    }
}

void ControllerDevice::setMissingProperties()
{
    stabilizePropertyString (Tags::uuid, Uuid().toString());
    stabilizePropertyString (Tags::name, "New Device");
}

}
