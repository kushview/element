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
    return parameterIndex == Processor::EnabledParameter
           || parameterIndex == Processor::BypassParameter
           || parameterIndex == Processor::MuteParameter;
}

void ParameterTarget::apply (const juce::MidiMessage& message, bool toggle)
{
    if (! isValid())
        return;
    if (parameter != nullptr)
        applyToParameter (message, toggle);
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

//=============================================================================
std::unique_ptr<MappingTarget> createTarget (const MidiMapping& mapping, Session& session)
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

    // TODO: "tempo" / "transport" targets (see docs/plans/midimapping.md open questions)
    return nullptr;
}

} // namespace element
