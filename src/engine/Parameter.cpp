
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

    ScopedLock lock (listenerLock);

    for (int i = listeners.size(); --i >= 0;)
        if (auto* l = listeners[i])
            l->parameterGestureChanged (getParameterIndex(), true);
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

    ScopedLock lock (listenerLock);

    for (int i = listeners.size(); --i >= 0;)
        if (auto* l = listeners[i])
            l->parameterGestureChanged (getParameterIndex(), false);
}

void Parameter::sendValueChangedMessageToListeners (float newValue)
{
    ScopedLock lock (listenerLock);

    for (int i = listeners.size(); --i >= 0;)
        if (auto* l = listeners [i])
            l->parameterValueChanged (getParameterIndex(), newValue);
}

bool Parameter::isOrientationInverted() const                      { return false; }
bool Parameter::isAutomatable() const                              { return true; }
bool Parameter::isMetaParameter() const                            { return false; }
Parameter::Category Parameter::getCategory() const             { return genericParameter; }
int Parameter::getNumSteps() const                                 { return defaultNumSteps(); }
bool Parameter::isDiscrete() const                                 { return false; }
bool Parameter::isBoolean() const                                  { return false; }

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
    : port (p),
      range (p.minValue, p.maxValue),
      value (p.defaultValue)
{}

ControlPortParameter::~ControlPortParameter() {}

void ControlPortParameter::setPort (const kv::PortDescription& newPort)
{
    port = newPort;
}

}
