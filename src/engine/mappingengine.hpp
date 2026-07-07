// SPDX-FileCopyrightText: 2023 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <element/juce.hpp>
#include <element/midimapping.hpp>
#include <element/session.hpp>
#include <element/signals.hpp>
#include <element/taptempo.hpp>

namespace element {

class MappingTarget;
class Processor;
class Node;
class MidiEngine;

/** Routes incoming MIDI to flat MidiMapping targets and drives the Learn
    capture flow. See docs/plans/midimapping.md. */
class MappingEngine
{
public:
    using CapturedEventSignal = Signal<void()>;
    using TempoTapSignal = Signal<void()>;

    MappingEngine();
    ~MappingEngine();

    /** Rebuild live bindings from the session's flat MIDI mappings. */
    void rebuildBindings (SessionPtr session);

    /** Route or capture a single incoming MIDI message. Intended to be called
        on the message thread; this is the unit-test entry point. */
    void process (const juce::String& device, const juce::MidiMessage& message);

    /** Begin/stop receiving MIDI from all inputs and routing through process().
        Messages are marshalled to the message thread. */
    void startListening (MidiEngine& midi);
    void stopListening (MidiEngine& midi);

    /** Shared tap-tempo accumulator driving both the UI TAP button and MIDI
        tap-tempo mappings; only the call site (thread/location) differs. Call on
        the message thread. Returns the newly computed BPM, or nullopt on the
        first tap of a run. */
    std::optional<double> tapTempo (double timeMs) { return tempoTap.tap (timeMs); }
    void resetTapTempo() { tempoTap.reset(); }

    /** Arm/disarm capture of the next incoming message. */
    void captureMapping (bool shouldCapture) { mapCapture.set (shouldCapture); }
    bool isCapturingMapping() const { return mapCapture.get(); }
    juce::String getCapturedDevice() const { return capturedDevice; }
    juce::MidiMessage getCapturedMessage() const { return capturedMessage; }
    CapturedEventSignal& mappingCapturedSignal() { return mapCapturedCallback; }

    /** Fired on each recognised MIDI tap-tempo event so the UI can flash the
        TAP button. Long-lived (outlives the per-rebuild targets that fire it). */
    TempoTapSignal& tempoTapAppliedSignal() { return tempoTapCallback; }

private:
    struct Binding;
    class Router;
    std::vector<std::unique_ptr<Binding>> bindings;
    std::unique_ptr<Router> router;
    juce::Atomic<bool> mapCapture { false };
    juce::String capturedDevice;
    juce::MidiMessage capturedMessage;
    CapturedEventSignal mapCapturedCallback;
    TempoTapSignal tempoTapCallback;
    TapTempo tempoTap; // shared by UI + MIDI tap tempo
};

} // namespace element
