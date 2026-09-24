// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <element/midimapping.hpp>
#include <element/processor.hpp>
#include <element/services.hpp>
#include <element/signals.hpp>
#include <element/transport.hpp>

namespace element {

class Node;

class MappingService : public Service
{
public:
    MappingService();
    ~MappingService();

    void activate() override;
    void deactivate() override;
    void learn (const bool shouldLearn = true);

    /** Register a tap from the UI TAP button and apply it to the session tempo.
        Shares the same accumulator as MIDI tap-tempo mappings; only the call
        site differs. */
    void tapTempo();

    /** Arm capture for a tap-tempo mapping: the next incoming MIDI event is
        bound to the session tempo (no parameter-wiggle phase). */
    void learnTempo();

    /** True if the session has a tap-tempo mapping. */
    bool hasTempoMapping();

    /** Short human label for the current tap-tempo mapping's trigger
        (e.g. "Note 60"), or empty when none exists. */
    juce::String getTempoMappingDescription();

    /** Remove any tap-tempo mapping(s) and refresh. */
    void clearTempoMapping();

    /** Arm capture for a transport mapping: the next incoming MIDI event is
        bound to the given transport action (no parameter-wiggle phase). */
    void learnTransport (TransportAction action);

    /** True if the session has a mapping for the given transport action. */
    bool hasTransportMapping (TransportAction action);

    /** Short human label for the given transport action's mapping trigger
        (e.g. "CC 21"), or empty when none exists. */
    juce::String getTransportMappingDescription (TransportAction action);

    /** Remove any mapping(s) for the given transport action and refresh. */
    void clearTransportMapping (TransportAction action);

    bool isLearning() const;
    void remove (const MidiMapping&);

    using MappingPredicate = std::function<bool (const MidiMapping&)>;

    /** Fired on the message thread each time a MIDI tap-tempo mapping is
        triggered, so the UI can flash the TAP button. */
    Signal<void()> sigTempoTapApplied;

    /** Rebuild live engine bindings from the current session. Call after
        editing a mapping in place so the change takes effect immediately. */
    void refresh();

private:
    class Impl;
    friend class Impl;
    std::unique_ptr<Impl> impl;
    SignalConnection capturedConnection;
    SignalConnection capturedParamConnection;
    SignalConnection devicesChangedConnection;
    SignalConnection sessionLoadedConnection;
    SignalConnection tempoTapConnection;
    SignalConnection transportActionConnection;
    void onControlCaptured();
    void onParameterCaptured (const Node&, int);
    void onTempoTapApplied() { sigTempoTapApplied(); }

    /** First session mapping satisfying the predicate, or an invalid mapping. */
    MidiMapping findMapping (const MappingPredicate& predicate);

    /** Remove every session mapping satisfying the predicate, then rebuild
        bindings and refresh views if anything was removed. */
    void removeMappings (const MappingPredicate& predicate);

    /** "Note 60" / "CC 21" for a mapping's trigger, or empty if invalid. */
    static juce::String describeTrigger (const MidiMapping& mapping);

    /** Skip the parameter-capture phase and arm MIDI capture for a session
        level target ("tempo" or "transport"). */
    void armSessionCapture (const juce::String& targetType);

    /** Add a newly learned mapping, make it live and refresh views. */
    void addLearnedMapping (const MidiMapping& mapping);

    /** Store the current human-readable name for every mapping whose MIDI input
        device is connected, so a later disconnect can still show a friendly name
        instead of the raw device identifier. */
    void syncDeviceNames();
};

} // namespace element
