// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_basics.hpp>

#include <element/model.hpp>
#include <element/tags.hpp>

#define EL_CONTROL_VERSION    1
#define EL_CONTROLLER_VERSION 1

namespace element {

// FIXME:
using namespace juce;
class Controller;

class Control : public Model {
public:
    explicit Control (const ValueTree& data = ValueTree())
        : Model (data)
    {
        if (data.isValid())
            setMissingProperties();
    }

    enum ToggleMode : int {
        toggleEqualsOrHigher = 0,
        toggleEquals
    };

    Control (const String& name)
        : Model (types::Control, EL_CONTROL_VERSION)
    {
        setName (name);
        setMissingProperties();
    }

    ~Control() noexcept {}

    bool isValid() const { return objectData.isValid() && objectData.hasType (types::Control); }

    EL_MODEL_GETTER (getName, tags::name);
    EL_MODEL_SETTER (Name, tags::name);
    EL_MODEL_GETTER (getControlType, tags::type);

    juce::MidiMessage getMappingData() const
    {
        jassertfalse; // don't use this !
        return getMidiMessage();
    }

    juce::MidiMessage getMidiMessage() const
    {
        juce::MidiMessage midi;

        if (isNoteEvent()) {
            midi = juce::MidiMessage::noteOn (1, getEventId(), (uint8) 64);
        } else if (isControllerEvent()) {
            midi = juce::MidiMessage::controllerEvent (1, getEventId(), 64);
        }

        return midi;
    }

    bool isNoteEvent() const { return getProperty ("eventType").toString() == "note"; }
    bool isControllerEvent() const { return getProperty ("eventType").toString() == "controller"; }
    int getEventId() const { return (int) getProperty ("eventId", 0); }

    bool isMomentary() const { return (bool) getProperty ("momentary", false); }
    Value getMomentaryValue() { return getPropertyAsValue ("momentary"); }
    int getToggleValue() const { return (int) getProperty ("toggleValue", 0); }
    Value getToggleValueObject() { return getPropertyAsValue ("toggleValue"); }
    bool inverseToggle() const { return (bool) getProperty ("inverseToggle", false); }
    Value getInverseToggleObject() { return getPropertyAsValue ("inverseToggle"); }

    static ToggleMode toggleMode (const String& str)
    {
        if (str == "eqorhi")
            return toggleEqualsOrHigher;
        else if (str == "eq")
            return toggleEquals;
        return toggleEqualsOrHigher;
    }

    ToggleMode toggleMode() const noexcept
    {
        return toggleMode (getProperty ("toggleMode").toString());
    }

    Value toggleModeObject() { return getPropertyAsValue ("toggleMode"); }

    Controller controller() const;

    String getUuidString() const { return objectData.getProperty (tags::uuid).toString(); }

private:
    juce::MidiMessage getMappingDataLegacy() const
    {
        const var& data (objectData.getProperty (tags::mappingData));
        if (const auto* block = data.getBinaryData())
            if (block->getSize() > 0)
                return juce::MidiMessage (block->getData(), (int) block->getSize());
        return juce::MidiMessage();
    }

    void setMissingProperties()
    {
        stabilizePropertyString (tags::name, "Control");
        stabilizePropertyString (tags::uuid, Uuid().toString());

        if (hasProperty (tags::mappingData)) {
            const auto midi = getMappingDataLegacy();
            if (midi.isNoteOnOrOff()) {
                setProperty ("eventType", "note");
                setProperty ("eventId", midi.getNoteNumber());
            } else if (midi.isController()) {
                setProperty ("eventType", "controller");
                setProperty ("eventId", midi.getControllerNumber());
            }
            objectData.removeProperty (tags::mappingData, nullptr);
        }

        stabilizePropertyString ("eventType", "controller");
        stabilizePropertyPOD ("momentary", false);
        stabilizePropertyPOD ("eventId", 0);
        stabilizePropertyPOD (tags::midiChannel, 0);
        stabilizePropertyPOD ("toggleValue", 64);
        stabilizePropertyPOD ("inverseToggle", false);
        stabilizePropertyString ("toggleMode", "eqorhi");
    }
};

class Controller : public Model {
public:
    explicit Controller (const ValueTree& data = ValueTree());
    Controller (const String& name);
    virtual ~Controller() {}

    inline bool isValid() const { return objectData.isValid() && objectData.hasType (types::Controller); }

    EL_MODEL_GETTER (getName, tags::name)
    EL_MODEL_SETTER (Name, tags::name)
    EL_MODEL_GETTER (getInputDevice, tags::inputDevice)
    inline String getUuidString() const { return objectData.getProperty (tags::uuid).toString(); }

    inline int getNumControls() const { return data().getNumChildren(); }
    inline Control getControl (const int index) const
    {
        const auto child (objectData.getChild (index));
        return Control (child);
    }

    inline int indexOf (const Model& model) const { return objectData.indexOf (model.data()); }
    inline int indexOf (const Identifier& childType, const Model& model) const
    {
        return objectData.getChildWithName (childType).indexOf (model.data());
    }

    inline Control findControlById (const Uuid& uuid) const
    {
        const Control control (objectData.getChildWithProperty (tags::uuid, uuid.toString()));
        return control;
    }

private:
    void setMissingProperties();
};

class ControllerMap : public Model {
public:
    explicit ControllerMap (const ValueTree& data = ValueTree()) : Model (data) {}
    ~ControllerMap() noexcept {}
    inline bool isValid() const { return objectData.isValid() && objectData.hasType (tags::map); }
    inline int getParameterIndex() const { return (int) objectData.getProperty (tags::parameter, -1); }
};

} // namespace element
