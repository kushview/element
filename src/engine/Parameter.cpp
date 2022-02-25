/*
    This file is part of Element

    Copyright (c) 2017 - 2019 ROLI Ltd.
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "engine/Parameter.h"

namespace Element {

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

ControlPortParameter::ControlPortParameter (const kv::PortDescription& p)
{
    jassert (p.type == kv::PortType::Control);
    setPort (p);
}

ControlPortParameter::~ControlPortParameter() {}

String ControlPortParameter::getText (float normalisedValue, int /*maxLength*/) const
{
    return String (convertFrom0to1 (normalisedValue), 6);
}

void ControlPortParameter::setPort (const kv::PortDescription& newPort, bool preserveValue)
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

ControlPortParameter& ControlPortParameter::operator= (float newValue)
{
    if (value != newValue)
        setValueNotifyingHost (convertTo0to1 (newValue));
    return *this;
}

} // namespace Element
