// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/parameter.hpp>

using namespace juce;
namespace element {

//==============================================================================
Parameter::Parameter() noexcept {}

Parameter::~Parameter()
{
#if JUCE_DEBUG && ! JUCE_DISABLE_AUDIOPROCESSOR_BEGIN_END_GESTURE_CHECKING
    // This will fail if you've called beginChangeGesture() without having made
    // a corresponding call to endChangeGesture...
    jassert (! isPerformingGesture);
#endif
}

void Parameter::setValueNotifyingHost (float newValue)
{
    setValue (newValue);
    sendValueChangedMessageToListeners (newValue);
}

void Parameter::beginChangeGesture()
{
    // This method can't be used until the parameter has been attached to a processor!
    jassert (parameterIndex >= 0);

#if JUCE_DEBUG && ! JUCE_DISABLE_AUDIOPROCESSOR_BEGIN_END_GESTURE_CHECKING
    // This means you've called beginChangeGesture twice in succession without
    // a matching call to endChangeGesture. That might be fine in most hosts,
    // but it would be better to avoid doing it.
    jassert (! isPerformingGesture);
    isPerformingGesture = true;
#endif

    sendGestureChangedMessageToListeners (true);
}

void Parameter::endChangeGesture()
{
    // This method can't be used until the parameter has been attached to a processor!
    jassert (parameterIndex >= 0);

#if JUCE_DEBUG && ! JUCE_DISABLE_AUDIOPROCESSOR_BEGIN_END_GESTURE_CHECKING
    // This means you've called endChangeGesture without having previously
    // called beginChangeGesture. That might be fine in most hosts, but it
    // would be better to keep the calls matched correctly.
    jassert (isPerformingGesture);
    isPerformingGesture = false;
#endif

    sendGestureChangedMessageToListeners (false);
}

void Parameter::sendValueChangedMessageToListeners (float newValue)
{
    ScopedLock lock (listenerLock);
    for (int i = listeners.size(); --i >= 0;)
        if (auto* l = listeners[i])
            l->controlValueChanged (getParameterIndex(), newValue);
}

void Parameter::sendGestureChangedMessageToListeners (bool touched)
{
    ScopedLock lock (listenerLock);
    for (int i = listeners.size(); --i >= 0;)
        if (auto* l = listeners[i])
            l->controlTouched (getParameterIndex(), touched);
}

bool Parameter::isOrientationInverted() const { return false; }
bool Parameter::isAutomatable() const { return true; }
bool Parameter::isMetaParameter() const { return false; }
Parameter::Category Parameter::getCategory() const { return genericParameter; }
int Parameter::getNumSteps() const { return defaultNumSteps(); }
bool Parameter::isDiscrete() const { return false; }
bool Parameter::isBoolean() const { return false; }

String Parameter::getText (float value, int) const
{
    return String (value, 2);
}

String Parameter::getCurrentValueAsText() const
{
    return getText (getValue(), 1024);
}

StringArray Parameter::getValueStrings() const
{
    if (isDiscrete() && valueStrings.isEmpty())
    {
        auto maxIndex = getNumSteps() - 1;

        for (int i = 0; i < getNumSteps(); ++i)
            valueStrings.add (getText ((float) i / maxIndex, 1024));
    }

    return valueStrings;
}

void Parameter::addListener (Parameter::Listener* newListener)
{
    const ScopedLock sl (listenerLock);
    listeners.addIfNotAlreadyThere (newListener);
}

void Parameter::removeListener (Parameter::Listener* listenerToRemove)
{
    const ScopedLock sl (listenerLock);
    listeners.removeFirstMatchingValue (listenerToRemove);
}

RangedParameter::RangedParameter (const PortDescription& p)
{
    jassert (p.type == PortType::Control);
    setPort (p);
}

RangedParameter::~RangedParameter() {}

String RangedParameter::getText (float normalisedValue, int /*maxLength*/) const
{
    return String (convertFrom0to1 (normalisedValue), 6);
}

void RangedParameter::setPort (const PortDescription& newPort, bool preserveValue)
{
    port = newPort;
    range.start = port.minValue;
    range.end = port.maxValue;
    if (preserveValue)
    {
        operator= (jlimit (range.start, range.end, value));
    }
    else
    {
        operator= (port.defaultValue);
    }
}

RangedParameter& RangedParameter::operator= (float newValue) noexcept
{
    if (value != newValue)
        setValueNotifyingHost (convertTo0to1 (newValue));
    return *this;
}

} // namespace element
