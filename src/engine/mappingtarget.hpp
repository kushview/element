// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <element/juce/audio_basics.hpp>
#include <element/node.hpp>
#include <element/parameter.hpp>
#include <element/signals.hpp>
#include <element/taptempo.hpp>
#include <element/transport.hpp>

namespace element {

class MidiMapping;
class Session;

/** Applies an incoming MIDI message to some destination (a node parameter,
    tempo, transport, ...).

    Targets are pure with respect to MIDI hardware: apply() is intended to be
    called on the message thread and can be unit tested by feeding messages
    directly.
*/
class MappingTarget
{
public:
    virtual ~MappingTarget() = default;

    /** True if this target can currently be applied. */
    virtual bool isValid() const = 0;

    /** Apply a matching MIDI message.
        @param message  The incoming note or controller message.
        @param toggle   For note events: latch on each note-on vs momentary.
    */
    virtual void apply (const juce::MidiMessage& message, bool toggle) = 0;
};

//=============================================================================
/** Targets a node parameter (regular index, or a special Enabled/Bypass/Mute or
    Input/Output gain parameter). */
class ParameterTarget : public MappingTarget
{
public:
    ParameterTarget (const Node& node, int parameterIndex);
    ~ParameterTarget() override = default;

    bool isValid() const override;
    void apply (const juce::MidiMessage& message, bool toggle) override;

private:
    Node model;
    ProcessorPtr object;
    ParameterPtr parameter; // null when targeting a special parameter
    int parameterIndex { -1 };

    void applyToParameter (const juce::MidiMessage&, bool toggle);
    void applySpecial (const juce::MidiMessage&, bool toggle);
    void applyGain (const juce::MidiMessage&, bool toggle);
};

//=============================================================================
/** Decides whether a note or controller message counts as a press for a
    trigger-style target. Notes fire on note-on only; controllers fire on the
    edge defined by MidiMapping::isTriggerEdge(), tracking the last value so a
    held knob fires only once. */
struct TriggerDetector
{
    juce::String mode { "above" };
    int value { 67 };
    int lastControllerValue { -1 };

    /** @return true if the message counts as a press. */
    bool accept (const juce::MidiMessage& message);
};

//=============================================================================
/** Targets the session tempo as a tap-tempo control: each matching note-on, or
    each recognised controller edge, counts as a tap and the averaged BPM is
    written to the session, which the audio engine picks up on the message
    thread. */
class TempoTarget : public MappingTarget
{
public:
    /** @param sessionData     The session tree whose tempo is written.
        @param tapTempo         Shared tap-tempo accumulator (owned by MappingEngine)
                                so UI and MIDI taps contribute to the same state.
        @param tempoTapApplied  Fired on every recognised tap so the UI can flash
                                the TAP button; owned by MappingEngine, so it
                                outlives this target.
        @param triggerMode      Controller trigger mode, see MidiMapping::isTriggerEdge().
        @param triggerValue     Threshold for the "above" trigger mode. */
    TempoTarget (const juce::ValueTree& sessionData,
                 TapTempo& tapTempo,
                 Signal<void()>& tempoTapApplied,
                 const juce::String& triggerMode = "above",
                 int triggerValue = 67);
    ~TempoTarget() override = default;

    bool isValid() const override;
    void apply (const juce::MidiMessage& message, bool toggle) override;

private:
    juce::ValueTree session;
    TapTempo& tapTempo;
    Signal<void()>& tempoTapApplied;
    TriggerDetector trigger;
};

//=============================================================================
/** Targets the audio transport: each recognised press performs one
    TransportAction. The action is delivered through a signal owned by
    MappingEngine and routed to the AudioEngine by MappingService, so this
    layer stays free of engine and hardware dependencies. */
class TransportTarget : public MappingTarget
{
public:
    /** @param action           The action performed on each press.
        @param transportAction  Fired with the action on every recognised press;
                                owned by MappingEngine, so it outlives this target.
        @param triggerMode      Controller trigger mode, see MidiMapping::isTriggerEdge().
        @param triggerValue     Threshold for the "above" trigger mode. */
    TransportTarget (TransportAction action,
                     Signal<void (TransportAction)>& transportAction,
                     const juce::String& triggerMode = "above",
                     int triggerValue = 67);
    ~TransportTarget() override = default;

    bool isValid() const override { return true; }
    void apply (const juce::MidiMessage& message, bool toggle) override;

private:
    TransportAction action;
    Signal<void (TransportAction)>& transportAction;
    TriggerDetector trigger;
};

//=============================================================================
/** Resolves a MidiMapping into a concrete target. Returns nullptr if the
    mapping cannot currently be resolved (e.g. missing node).
    @param tapTempo         Shared accumulator passed to a TempoTarget, if created.
    @param tempoTapApplied  Flash signal forwarded to a TempoTarget, if created.
    @param transportAction  Action signal forwarded to a TransportTarget, if created. */
std::unique_ptr<MappingTarget> createTarget (const MidiMapping& mapping,
                                             Session& session,
                                             TapTempo& tapTempo,
                                             Signal<void()>& tempoTapApplied,
                                             Signal<void (TransportAction)>& transportAction);

} // namespace element
