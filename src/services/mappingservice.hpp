// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/services.hpp>
#include <element/processor.hpp>
#include <element/midimapping.hpp>
#include <element/signals.hpp>

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

    bool isLearning() const;
    void remove (const MidiMapping&);

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
    void onControlCaptured();
    void onParameterCaptured (const Node&, int);

    /** Store the current human-readable name for every mapping whose MIDI input
        device is connected, so a later disconnect can still show a friendly name
        instead of the raw device identifier. */
    void syncDeviceNames();
};

} // namespace element
