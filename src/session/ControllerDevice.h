/*
    ControllerDevice.h - This file is part of Element
    Copyright (C) 2018  Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Node.h"

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
        explicit Control (const ValueTree& data = ValueTree()) 
            : ObjectModel (data) 
        {
            if (data.isValid())
                setMissingProperties();
        }

        ~Control() noexcept { }

        bool isValid() const { return objectData.isValid() && objectData.hasType (Tags::control); }

        EL_OBJECT_GETTER_AND_SETTER(Name, Tags::name);
        EL_OBJECT_GETTER(ControlType, Tags::type);

        MidiMessage getMappingData() const
        {
            const var& data (objectData.getProperty (Tags::mappingData));
            if (const auto* block = data.getBinaryData())
                if (block->getSize() > 0)
                    return MidiMessage (block->getData(), block->getSize());
            return MidiMessage();
        }

        ControllerDevice getControllerDevice() const
        {
            const ControllerDevice device (objectData.getParent());
            return device;
        }

        String getUuidString() const { return objectData.getProperty(Tags::uuid).toString(); }

    private:
        void setMissingProperties()
        {
            stabilizePropertyString (Tags::name, "Control");
            stabilizePropertyString (Tags::uuid, Uuid().toString());
        }
    };

    explicit ControllerDevice (const ValueTree& data = ValueTree());
    ControllerDevice (const String& name);
    virtual ~ControllerDevice() { }

    inline bool isValid() const { return objectData.isValid() && objectData.hasType (Tags::controller); }

    EL_OBJECT_GETTER_AND_SETTER(Name, Tags::name)
    EL_OBJECT_GETTER(InputDevice, "inputDevice")
    inline String getUuidString() const { return objectData.getProperty(Tags::uuid).toString(); }

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

    inline Control findControlById (const Uuid& uuid) const
    {
        const Control control (objectData.getChildWithProperty (Tags::uuid, uuid.toString()));
        return control;
    }

private:
    void setMissingProperties();
};

class ControllerMap : public ObjectModel
{
public:

    explicit ControllerMap (const ValueTree& data = ValueTree()) : ObjectModel (data) { }
    ~ControllerMap() noexcept { }
    inline bool isValid() const { return objectData.isValid() && objectData.hasType (Tags::map); }
    inline int getParameterIndex() const  { return (int) objectData.getProperty (Tags::parameter, -1); }
};

}
