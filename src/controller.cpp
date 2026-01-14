// Copyright 2018-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/controller.hpp>

namespace element {
using namespace juce;

Controller::Controller (const ValueTree& data)
    : Model (data)
{
    if (data.isValid())
    {
        jassert (data.hasType (types::Controller));
        jassert (data.hasProperty (tags::uuid));
        setMissingProperties();
    }
}

Controller::Controller (const String& name)
    : Model (types::Controller, EL_CONTROLLER_VERSION)
{
    setName (name);
    setMissingProperties();
}

void Controller::setMissingProperties()
{
    stabilizePropertyString (tags::uuid, Uuid().toString());
    stabilizePropertyString (tags::name, "New Device");
    stabilizePropertyString (tags::inputDevice, "");
}

Controller Control::controller() const
{
    const Controller device (objectData.getParent());
    return device;
}

} // namespace element
