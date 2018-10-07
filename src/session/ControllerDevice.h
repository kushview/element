/*
    ControllerDevice.h - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"

#define EL_OBJECT_GETTER(a,b) inline const var& get##a () const { return objectData.getProperty(b); }
#define EL_OBJECT_SETTER(a,b) inline void set##a (const var& value) { objectData.setProperty (b, value, nullptr); }
#define EL_OBJECT_GETTER_AND_SETTER(a,b) EL_OBJECT_GETTER(a,b) EL_OBJECT_SETTER(a,b)

namespace Element {

class ControllerDevice : public ObjectModel
{
public:
    class Control : public ObjectModel
    {
    public:
        Control() : ObjectModel (Tags::controller) { }
        Control (const ValueTree& data) : ObjectModel (data) { }
        ~Control() noexcept { }

        EL_OBJECT_GETTER_AND_SETTER(Name, Tags::name);
        EL_OBJECT_GETTER(ControlType, Tags::type);
    };

    ControllerDevice();
    ControllerDevice (const ValueTree& data);
    virtual ~ControllerDevice() { }

    EL_OBJECT_GETTER_AND_SETTER(Name, Tags::name)
    EL_OBJECT_GETTER(InputDevice, "inputDevice")

    inline int getNumControls() const { return getNumChildren(); }
    inline Control getControl (const int index) const
    {
        const auto child (objectData.getChild (index));
        return Control (child);
    }

    inline int indexOf (const ObjectModel& model) const { return objectData.indexOf (model.getValueTree()); }
    inline int indexOf (const Identifier& childType, const ObjectModel& model) const {
        return objectData.getChildWithName (childType).indexOf (model.getValueTree());
    }

private:
    void setMissingProperties();
};

}
