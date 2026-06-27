// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce/audio_basics.hpp>

#include <element/model.hpp>
#include <element/tags.hpp>

#define EL_MIDIMAPPING_VERSION 1

namespace element {

/** A flat MIDI mapping: a single MIDI event (note or CC) on an input device
    bound directly to a target (a node parameter, tempo, transport, ...).

    Replaces the legacy Controller -> Control -> ControllerMap hierarchy.
*/
class MidiMapping : public Model {
public:
    explicit MidiMapping (const juce::ValueTree& data = juce::ValueTree())
        : Model (data)
    {
        if (data.isValid())
            setMissingProperties();
    }

    MidiMapping (const juce::String& mappingName)
        : Model (types::MidiMapping, EL_MIDIMAPPING_VERSION)
    {
        setProperty (tags::name, mappingName);
        setMissingProperties();
    }

    ~MidiMapping() = default;

    bool isValid() const { return objectData.isValid() && objectData.hasType (types::MidiMapping); }

    juce::String getUuidString() const { return objectData.getProperty (tags::uuid).toString(); }
    juce::String getName() const { return objectData.getProperty (tags::name).toString(); }

    //=========================================================================
    /** The JUCE MIDI input device identifier. Empty matches any device. */
    juce::String getDevice() const { return objectData.getProperty (tags::device).toString(); }
    juce::String getEventType() const { return objectData.getProperty (tags::eventType).toString(); }
    int getEventId() const { return (int) objectData.getProperty (tags::eventId, 0); }
    /** MIDI channel, 1-16, or 0 for omni. */
    int getMidiChannel() const { return (int) objectData.getProperty (tags::midiChannel, 0); }
    /** For note events: latch on each note-on vs momentary. */
    bool isToggle() const { return (bool) objectData.getProperty (tags::toggle, false); }

    bool isNoteEvent() const { return getEventType() == "note"; }
    bool isControllerEvent() const { return getEventType() == "controller"; }

    //=========================================================================
    juce::String getTargetType() const { return objectData.getProperty (tags::targetType).toString(); }
    juce::Uuid getNodeUuid() const { return juce::Uuid (objectData.getProperty (tags::node).toString()); }
    int getParameterIndex() const { return (int) objectData.getProperty (tags::parameter, -1); }

    //=========================================================================
    /** Returns true if the given MIDI message matches this mapping's event
        type, id and channel (channel 0 = omni). Device is not checked here. */
    bool matches (const juce::MidiMessage& msg) const
    {
        const int ch = getMidiChannel();
        if (ch != 0 && msg.getChannel() != ch)
            return false;
        if (isControllerEvent())
            return msg.isController() && msg.getControllerNumber() == getEventId();
        if (isNoteEvent())
            return msg.isNoteOnOrOff() && msg.getNoteNumber() == getEventId();
        return false;
    }

    /** Returns a representative MIDI message for this mapping. */
    juce::MidiMessage getMidiMessage() const
    {
        if (isNoteEvent())
            return juce::MidiMessage::noteOn (1, getEventId(), (juce::uint8) 64);
        if (isControllerEvent())
            return juce::MidiMessage::controllerEvent (1, getEventId(), 64);
        return {};
    }

    //=========================================================================
    /** Build a fully-formed mapping from a captured MIDI message and target.
        Used by the Learn flow and by tests. */
    static MidiMapping fromCapture (const juce::String& device,
                                    const juce::MidiMessage& msg,
                                    const juce::String& targetType,
                                    const juce::Uuid& node,
                                    int parameter)
    {
        MidiMapping m { juce::String() };
        m.setProperty (tags::device, device);

        if (msg.isController()) {
            m.setProperty (tags::eventType, "controller");
            m.setProperty (tags::eventId, msg.getControllerNumber());
        } else if (msg.isNoteOnOrOff()) {
            m.setProperty (tags::eventType, "note");
            m.setProperty (tags::eventId, msg.getNoteNumber());
        }

        m.setProperty (tags::targetType, targetType);
        m.setProperty (tags::node, node.toString());
        m.setProperty (tags::parameter, parameter);
        return m;
    }

private:
    void setMissingProperties()
    {
        stabilizePropertyString (tags::uuid, juce::Uuid().toString());
        stabilizePropertyString (tags::name, juce::String());
        stabilizePropertyString (tags::device, juce::String());
        stabilizePropertyString (tags::eventType, "controller");
        stabilizePropertyPOD (tags::eventId, 0);
        stabilizePropertyPOD (tags::midiChannel, 0);
        stabilizePropertyPOD (tags::toggle, false);
        stabilizePropertyString (tags::targetType, "parameter");
        stabilizePropertyString (tags::node, juce::String());
        stabilizePropertyPOD (tags::parameter, -1);
    }
};

} // namespace element
