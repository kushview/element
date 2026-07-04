// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/mappingtarget.hpp"

#include <element/midimapping.hpp>
#include <element/processor.hpp>
#include <element/session.hpp>

using namespace juce;

namespace element {

//=============================================================================
ParameterTarget::ParameterTarget (const Node& node, int paramIndex)
    : model (node),
      object (node.getObject()),
      parameterIndex (paramIndex)
{
    if (object != nullptr && isPositiveAndBelow (parameterIndex, object->getParameters().size()))
        parameter = object->getParameters()[parameterIndex];
}

bool ParameterTarget::isValid() const
{
    if (object == nullptr)
        return false;
    if (parameter != nullptr)
        return true;
    return Processor::isSpecialParameter (parameterIndex);
}

static bool isGainParameter (int parameterIndex)
{
    return parameterIndex == Processor::InputGainParameter
           || parameterIndex == Processor::OutputGainParameter;
}

void ParameterTarget::apply (const juce::MidiMessage& message, bool toggle)
{
    if (! isValid())
        return;
    if (parameter != nullptr)
        applyToParameter (message, toggle);
    else if (isGainParameter (parameterIndex))
        applyGain (message, toggle);
    else
        applySpecial (message, toggle);
}

void ParameterTarget::applyToParameter (const juce::MidiMessage& message, bool toggle)
{
    float newValue = parameter->getValue();

    if (message.isController())
    {
        newValue = static_cast<float> (message.getControllerValue()) / 127.0f;
    }
    else if (message.isNoteOnOrOff())
    {
        if (toggle)
        {
            if (message.isNoteOn())
                newValue = parameter->getValue() < 0.5f ? 1.0f : 0.0f;
            else
                return; // ignore note-off in toggle mode
        }
        else
        {
            newValue = message.isNoteOn() ? 1.0f : 0.0f;
        }
    }

    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (newValue);
    parameter->endChangeGesture();
}

void ParameterTarget::applySpecial (const juce::MidiMessage& message, bool toggle)
{
    bool on = false;

    if (message.isController())
    {
        on = message.getControllerValue() >= 64;
    }
    else if (message.isNoteOnOrOff())
    {
        if (toggle)
        {
            if (! message.isNoteOn())
                return; // ignore note-off in toggle mode
            // flip current state
            const bool current = parameterIndex == Processor::EnabledParameter  ? object->isEnabled()
                                 : parameterIndex == Processor::BypassParameter ? object->isSuspended()
                                                                                : model.isMuted();
            on = ! current;
        }
        else
        {
            on = message.isNoteOn();
        }
    }

    switch (parameterIndex)
    {
        case Processor::EnabledParameter:
            object->setEnabled (on);
            if (model.isEnabled() != object->isEnabled())
                model.setProperty (tags::enabled, object->isEnabled());
            break;
        case Processor::BypassParameter:
            object->suspendProcessing (on);
            if (model.isBypassed() != object->isSuspended())
                model.setProperty (tags::bypass, object->isSuspended());
            break;
        case Processor::MuteParameter:
            model.setMuted (on);
            break;
        default:
            break;
    }
}

void ParameterTarget::applyGain (const juce::MidiMessage& message, bool toggle)
{
    // dB range mirrors the default node channel-strip fader (see ChannelStripComponent).
    constexpr double minDb = -60.0;
    constexpr double maxDb = 6.0;

    const bool input = parameterIndex == Processor::InputGainParameter;
    const float current = input ? object->getInputGain() : object->getGain();

    float gain = current;

    if (message.isController())
    {
        const double norm = static_cast<double> (message.getControllerValue()) / 127.0;
        gain = static_cast<float> (Decibels::decibelsToGain (minDb + norm * (maxDb - minDb), minDb));
    }
    else if (message.isNoteOnOrOff())
    {
        // Notes drive gain to full (max) or silence, latching in toggle mode.
        const float full = static_cast<float> (Decibels::decibelsToGain (maxDb, minDb));
        if (toggle)
        {
            if (! message.isNoteOn())
                return; // ignore note-off in toggle mode
            gain = current > 0.0f ? 0.0f : full;
        }
        else
        {
            gain = message.isNoteOn() ? full : 0.0f;
        }
    }

    if (input)
    {
        object->setInputGain (gain);
        model.setProperty (tags::inputGain, gain);
    }
    else
    {
        object->setGain (gain);
        model.setProperty (tags::gain, gain);
    }
}

//=============================================================================
TempoTarget::TempoTarget (const juce::ValueTree& sessionData, TapTempo& shared)
    : session (sessionData), tapTempo (shared)
{
}

bool TempoTarget::isValid() const
{
    return session.isValid();
}

void TempoTarget::apply (const juce::MidiMessage& message, bool /*toggle*/)
{
    if (! isValid() || ! message.isNoteOn())
        return; // note-on only: each press is a tap

    // Tap at the message's arrival time (stamped by the router), not "now" on
    // the message thread, so the interval matches what the player performed.
    if (auto bpm = tapTempo.tap (message.getTimeStamp()))
        session.setProperty (tags::tempo, *bpm, nullptr);
}

//=============================================================================
std::unique_ptr<MappingTarget> createTarget (const MidiMapping& mapping, Session& session, TapTempo& tapTempo)
{
    const auto targetType = mapping.getTargetType();

    if (targetType == "parameter")
    {
        auto node = session.findNodeById (mapping.getNodeUuid());
        if (! node.isValid() || node.getObject() == nullptr)
            return nullptr;
        auto target = std::make_unique<ParameterTarget> (node, mapping.getParameterIndex());
        if (! target->isValid())
            return nullptr;
        return target;
    }

    if (targetType == "tempo")
    {
        auto target = std::make_unique<TempoTarget> (session.data(), tapTempo);
        if (! target->isValid())
            return nullptr;
        return target;
    }

    // TODO: "transport" targets (see docs/plans/midimapping.md open questions)
    return nullptr;
}

} // namespace element
