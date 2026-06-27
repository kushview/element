// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include <element/juce/audio_basics.hpp>
#include <element/node.hpp>
#include <element/parameter.hpp>

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
/** Targets a node parameter (regular index, or a special Enabled/Bypass/Mute
    parameter). */
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
};

//=============================================================================
/** Resolves a MidiMapping into a concrete target. Returns nullptr if the
    mapping cannot currently be resolved (e.g. missing node). */
std::unique_ptr<MappingTarget> createTarget (const MidiMapping& mapping, Session& session);

} // namespace element
