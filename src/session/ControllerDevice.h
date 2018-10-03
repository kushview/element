/*
    ControllerDevice.h - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

#define EL_OBJECT_GETTER(a,b) const var& get##a () const { return objectData.getProperty(b); }
#define EL_OBJECT_SETTER(a,b) void set##a (const var& value) { objectData.setProperty (b, value, nullptr); }
#define EL_OBJECT_GETTER_AND_SETTER(a,b) EL_OBJECT_GETTER(a,b) EL_OBJECT_SETTER(a,b)

namespace Element {

class ControllerDevice : public ObjectModel
{
public:
    ControllerDevice() : ObjectModel (Identifier ("controllerDevice")) { }
    virtual ~ControllerDevice() { getName(); }

    EL_OBJECT_GETTER_AND_SETTER(Name, Tags::name)
    EL_OBJECT_GETTER(InputDevice, "inputDevice")
};

}
