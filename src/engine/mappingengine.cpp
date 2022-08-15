/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "engine/nodeobject.hpp"
#include "engine/mappingengine.hpp"
#include "engine/midiengine.hpp"
#include "session/controllerdevice.hpp"
#include "session/node.hpp"

namespace element {

class ControllerMapHandler
{
public:
    ControllerMapHandler() {}
    virtual ~ControllerMapHandler() {}

    virtual bool wants (const MidiMessage& message) const = 0;
    virtual void perform (const MidiMessage& message) = 0;
};

struct MidiNoteControllerMap : public ControllerMapHandler,
                               public AsyncUpdater,
                               private Value::Listener
{
    MidiNoteControllerMap (const ControllerDevice::Control& ctl,
                           const MidiMessage& message,
                           const Node& _node,
                           const int _parameter)
        : control (ctl),
          model (_node),
          node (_node.getObject()),
          parameter (nullptr),
          parameterIndex (_parameter),
          noteNumber (message.getNoteNumber())
    {
        jassert (message.isNoteOn());
        jassert (node);

        channelObject = control.getPropertyAsValue (Tags::midiChannel);
        channelObject.addListener (this);
        valueChanged (channelObject);

        momentaryObject = control.getMomentaryValue();
        momentaryObject.addListener (this);
        valueChanged (momentaryObject);

        inverseObject = control.getInverseToggleObject();
        inverseObject.addListener (this);
        valueChanged (inverseObject);

        if (isPositiveAndBelow (parameterIndex, node->getParameters().size()))
        {
            parameter = node->getParameters()[parameterIndex];
            jassert (nullptr != parameter);
        }
    }

    ~MidiNoteControllerMap()
    {
        channelObject.removeListener (this);
    }

    bool checkNoteAndChannel (const MidiMessage& message) const
    {
        return message.getNoteNumber() == noteNumber && (channel.get() == 0 || (channel.get() > 0 && message.getChannel() == channel.get()));
    }

    bool wants (const MidiMessage& message) const override
    {
        bool wants = momentary.get() == 0
                         ? message.isNoteOn() && checkNoteAndChannel (message)
                         : message.isNoteOnOrOff() && checkNoteAndChannel (message);
        return wants;
    }

    void perform (const MidiMessage& message) override
    {
        const bool isInverse = inverse.get() == 1;

        {
            SpinLock::ScopedLockType sl (eventLock);
            lastEvent = message;
        }

        jassert (message.isNoteOnOrOff());

        if (parameter != nullptr)
        {
            parameter->beginChangeGesture();
            if (momentary.get() == 0)
            {
                parameter->setValueNotifyingHost (parameter->getValue() < 0.5 ? 1.f : 0.f);
            }
            else
            {
                const bool onOrOff = isInverse ? message.isNoteOff() : message.isNoteOn();
                parameter->setValueNotifyingHost (onOrOff ? 1.f : 0.f);
            }

            parameter->endChangeGesture();
        }
        else if (parameterIndex == NodeObject::EnabledParameter || parameterIndex == NodeObject::BypassParameter || parameterIndex == NodeObject::MuteParameter)
        {
            triggerAsyncUpdate();
        }
    }

    void handleAsyncUpdate() override
    {
        MidiMessage event;

        {
            SpinLock::ScopedLockType sl (eventLock);
            event = lastEvent;
        }

        if (momentary.get() == 0)
        {
            if (parameterIndex == NodeObject::EnabledParameter)
            {
                node->setEnabled (! node->isEnabled());
                model.setProperty (Tags::enabled, node->isEnabled());
            }
            else if (parameterIndex == NodeObject::BypassParameter)
            {
                node->suspendProcessing (! node->isSuspended());
                model.setProperty (Tags::bypass, node->isSuspended());
            }
            else if (parameterIndex == NodeObject::MuteParameter)
            {
                model.setMuted (! model.isMuted());
            }
        }
        else
        {
            jassert (event.isNoteOnOrOff());
            // DBG("async note off: " << (int) event.isNoteOff());
            const bool isInverse = inverse.get() == 1;

            if (parameterIndex == NodeObject::EnabledParameter)
            {
                node->setEnabled (isInverse ? event.isNoteOff() : event.isNoteOn());
                model.setProperty (Tags::enabled, node->isEnabled());
            }
            else if (parameterIndex == NodeObject::BypassParameter)
            {
                node->suspendProcessing (isInverse ? event.isNoteOn() : event.isNoteOff());
                model.setProperty (Tags::bypass, node->isSuspended());
            }
            else if (parameterIndex == NodeObject::MuteParameter)
            {
                model.setMuted (isInverse ? event.isNoteOff() : event.isNoteOn());
            }
        }
    }

private:
    ControllerDevice::Control control;
    Node model;
    NodeObjectPtr node { nullptr };
    Parameter::Ptr parameter { nullptr };
    int parameterIndex = -1;

    Value channelObject;
    Atomic<int> channel { 0 };

    Value momentaryObject;
    Atomic<int> momentary { 0 };

    Value inverseObject;
    Atomic<int> inverse { 0 };

    const int noteNumber;

    SpinLock eventLock;
    MidiMessage lastEvent;

    void valueChanged (Value& value) override
    {
        if (channelObject.refersToSameSourceAs (value))
        {
            channel.set (jlimit (0, 16, (int) channelObject.getValue()));
        }
        else if (momentaryObject.refersToSameSourceAs (value))
        {
            momentary.set ((bool) momentaryObject.getValue() ? 1 : 0);
        }
        else if (inverseObject.refersToSameSourceAs (value))
        {
            inverse.set ((bool) inverseObject.getValue() ? 1 : 0);
        }
    }
};

struct MidiCCControllerMapHandler : public ControllerMapHandler,
                                    public AsyncUpdater,
                                    private Value::Listener
{
    MidiCCControllerMapHandler (const ControllerDevice::Control& ctl,
                                const MidiMessage& message,
                                const Node& _node,
                                const int _parameter)
        : control (ctl), model (_node), node (_node.getObject()), parameter (nullptr), controllerNumber (message.getControllerNumber()), parameterIndex (_parameter)
    {
        jassert (message.isController());
        jassert (node != nullptr);

        toggleValueObject = control.getToggleValueObject();
        toggleValueObject.addListener (this);
        toggleValue.set (jlimit (0, 127, control.getToggleValue()));

        inverseToggleObject = control.getInverseToggleObject();
        inverseToggleObject.addListener (this);
        inverseToggle.set (control.inverseToggle() ? 1 : 0);

        toggleModeObject = control.getToggleModeObject();
        toggleModeObject.addListener (this);
        toggleMode.set (static_cast<int> (control.getToggleMode()));

        channelObject = control.getPropertyAsValue (Tags::midiChannel);
        channelObject.addListener (this);
        valueChanged (channelObject);

        if (isPositiveAndBelow (parameterIndex, node->getParameters().size()))
        {
            parameter = node->getParameters()[parameterIndex];
            jassert (nullptr != parameter);
        }
        else if (parameterIndex == NodeObject::EnabledParameter)
        {
            lastControllerValue = model.isEnabled() ? 127 : 0;
        }
    }

    ~MidiCCControllerMapHandler()
    {
        toggleValueObject.removeListener (this);
        inverseToggleObject.removeListener (this);
        toggleModeObject.removeListener (this);
        channelObject.removeListener (this);
    }

    bool wants (const MidiMessage& message) const override
    {
        return message.isController() && message.getControllerNumber() == controllerNumber && (channel.get() == 0 || (channel.get() > 0 && message.getChannel() == channel.get()));
    }

    void perform (const MidiMessage& message) override
    {
        const auto ccValue = message.getControllerValue();

        if (nullptr != parameter)
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (static_cast<float> (ccValue) / 127.f);
            parameter->endChangeGesture();
        }
        else if (parameterIndex == NodeObject::EnabledParameter || parameterIndex == NodeObject::BypassParameter || parameterIndex == NodeObject::MuteParameter)
        {
            const auto currentToggleState = desiredToggleState.get();
            const auto mode = toggleMode.get();

            if (mode == ControllerDevice::EqualsOrHigher)
            {
                if (toggleValue.get() == 0)
                {
                    if (lastControllerValue == 0 && ccValue > 0)
                    {
                        desiredToggleState.set (1);
                    }
                    else if (lastControllerValue > 0 && ccValue == 0)
                    {
                        desiredToggleState.set (0);
                    }
                }
                else if (toggleValue.get() == 127)
                {
                    if (lastControllerValue < 127 && ccValue == 127)
                    {
                        desiredToggleState.set (1);
                    }
                    else if (lastControllerValue == 127 && ccValue < 127)
                    {
                        desiredToggleState.set (0);
                    }
                }
                else
                {
                    if (lastControllerValue < toggleValue.get() && ccValue >= toggleValue.get())
                    {
                        desiredToggleState.set (1);
                    }
                    else if (lastControllerValue >= toggleValue.get() && ccValue < toggleValue.get())
                    {
                        desiredToggleState.set (0);
                    }
                }
            }
            else if (mode == ControllerDevice::Equals)
            {
                if (toggleValue.get() == ccValue)
                {
                    desiredToggleState.set (currentToggleState == 0 ? 1 : 0);
                }
            }
            else
            {
                jassertfalse;
                // toggle mode not supported
            }

            if (currentToggleState != desiredToggleState.get())
                triggerAsyncUpdate();
        }

        lastControllerValue = ccValue;
    }

    void handleAsyncUpdate() override
    {
        const auto mode = toggleMode.get();
        const int stateToCompare = mode != ControllerDevice::Equals
                                       ? (inverseToggle.get() == 1 ? 0 : 1) // inverse on, then compare false
                                       : 1; // equals mode always compare true

        if (parameterIndex == NodeObject::EnabledParameter)
        {
            node->setEnabled (desiredToggleState.get() == stateToCompare);
            if (model.isEnabled() != node->isEnabled())
                model.setProperty (Tags::enabled, node->isEnabled());
        }
        else if (parameterIndex == NodeObject::BypassParameter)
        {
            // inverted because UI displays bypass as inactive (or active for not bypassed)
            node->suspendProcessing (! (desiredToggleState.get() == stateToCompare));
            if (model.isBypassed() != node->isSuspended())
                model.setProperty (Tags::bypass, node->isSuspended());
        }
        else if (parameterIndex == NodeObject::MuteParameter)
        {
            model.setMuted (desiredToggleState.get() == stateToCompare);
        }
    }

private:
    ControllerDevice::Control control;
    Node model;
    NodeObjectPtr node { nullptr };
    Parameter::Ptr parameter { nullptr };

    const int controllerNumber { -1 };
    const int parameterIndex { -1 };
    int lastControllerValue = 0;

    Value toggleValueObject;
    Atomic<int> toggleValue { 64 };

    Value inverseToggleObject;
    Atomic<int> inverseToggle;

    Value toggleModeObject;
    Atomic<int> toggleMode { 0 };

    Value channelObject;
    Atomic<int> channel { 0 };

    Atomic<int> desiredToggleState { 1 };

    void valueChanged (Value& value) override
    {
        if (toggleValueObject.refersToSameSourceAs (value))
        {
            toggleValue.set (jlimit (0, 127, (int) toggleValueObject.getValue()));
        }
        else if (inverseToggleObject.refersToSameSourceAs (value))
        {
            inverseToggle.set ((bool) value.getValue() ? 1 : 0);
        }
        else if (toggleModeObject.refersToSameSourceAs (value))
        {
            toggleMode.set (static_cast<int> (ControllerDevice::Control::getToggleMode (
                toggleModeObject.getValue().toString())));
        }
        else if (channelObject.refersToSameSourceAs (value))
        {
            channel.set (jlimit (0, 16, (int) channelObject.getValue()));
        }
    }
};

class ControllerMapInput : public MidiInputCallback
{
public:
    explicit ControllerMapInput (MappingEngine& owner, MidiEngine& m, const ControllerDevice& device)
        : midi (m), mapping (owner), controllerDevice (device)
    {
    }

    ~ControllerMapInput()
    {
        close();
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message)
    {
        if ((! message.isController() || ! controllerNumbers[message.getControllerNumber()]) && (! message.isNoteOnOrOff() || ! noteNumbers[message.getNoteNumber()]))
            return;

        // DBG("[EL] handle mapped MIDI: " << message.getControllerNumber()
        //     << " : " << message.getControllerValue());
        if (message.isNoteOn())
            mapping.captureNextEvent (*this, notes[message.getNoteNumber()], message);
        else if (message.isController())
            mapping.captureNextEvent (*this, controls[message.getControllerNumber()], message);

        for (auto* handler : handlers)
            if (handler->wants (message))
                handler->perform (message);
    }

    bool close()
    {
        const auto deviceName = controllerDevice.getInputDevice().toString();
        midi.removeMidiInputCallback (this);
        return true;
    }

    bool open()
    {
        close();

        for (int i = controllerDevice.getNumControls(); --i >= 0;)
        {
            const auto control (controllerDevice.getControl (i));
            const auto midi (control.getMidiMessage());
            if (midi.isController())
            {
                controllerNumbers.setBit (midi.getControllerNumber(), true);
                controls.set (midi.getControllerNumber(), control);
            }
            else if (midi.isNoteOn())
            {
                noteNumbers.setBit (midi.getNoteNumber(), true);
                notes.set (midi.getNoteNumber(), control);
            }
        }

        const auto deviceName = controllerDevice.getInputDevice().toString();
        midi.addMidiInputCallback (deviceName, this, true);

        return true;
    }

    void start()
    {
        open();
    }

    void stop()
    {
        close();
    }

    bool isInputFor (const ControllerDevice& device) const
    {
        return device.getValueTree() == controllerDevice.getValueTree();
    }

    bool isInputFor (const ControllerDevice::Control& control) const
    {
        return isInputFor (control.getControllerDevice());
    }

    void addHandler (ControllerMapHandler* handler)
    {
        stop();
        handlers.add (handler);
        start();
    }

private:
    MidiEngine& midi;
    MappingEngine& mapping;
    ControllerDevice controllerDevice;
    std::unique_ptr<MidiInput> midiInput;
    OwnedArray<ControllerMapHandler> handlers;
    BigInteger controllerNumbers, noteNumbers;
    HashMap<int, ControllerDevice::Control> controls, notes;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllerMapInput)
};

class MappingEngine::Inputs
{
public:
    Inputs() {}
    ~Inputs() {}

    bool add (ControllerMapInput* input)
    {
        if (inputs.contains (input))
            return true;
        inputs.add (input);
        if (isRunning())
            input->start();
        return inputs.contains (input);
    }

    bool remove (const ControllerDevice& device)
    {
        if (auto* input = findInput (device))
        {
            input->close();
            inputs.removeObject (input, true);
        }

        return ! containsInputFor (device);
    }

    void clear()
    {
        stop();
        for (auto* input : inputs)
            input->close();
        inputs.clear (true);
    }

    void start()
    {
        if (isRunning())
            stop();

        for (auto* input : inputs)
            input->start();

        running = true;
    }

    void stop()
    {
        running = false;
        for (auto* input : inputs)
            input->stop();
    }

    ControllerMapInput* findInput (const ControllerDevice& controller) const
    {
        if (! controller.isValid())
            return nullptr;
        for (auto* const input : inputs)
            if (input->isInputFor (controller))
                return input;
        return nullptr;
    }

    bool containsInputFor (const ControllerDevice& controller) const
    {
        return findInput (controller) != nullptr;
    }

    bool isRunning() const { return running; }

    ControllerMapInput* const* begin() const noexcept { return inputs.begin(); }
    ControllerMapInput* const* end() const noexcept { return inputs.end(); }
    int size() const noexcept { return inputs.size(); }

    void swapWith (OwnedArray<ControllerMapInput>& other) { inputs.swapWith (other); }

private:
    OwnedArray<ControllerMapInput> inputs;
    bool running = false;
};

MappingEngine::MappingEngine()
{
    inputs.reset (new Inputs());
    capturedEvent.capture.set (true);
}

MappingEngine::~MappingEngine()
{
    inputs->clear();
    inputs = nullptr;
}

bool MappingEngine::addInput (const ControllerDevice& controller, MidiEngine& midi)
{
    if (inputs->containsInputFor (controller))
        return true;

    std::unique_ptr<ControllerMapInput> input;
    input.reset (new ControllerMapInput (*this, midi, controller));

    DBG ("[EL] MappingEngine: added input handler for controller: " << controller.getName().toString());
    return inputs->add (input.release());
}

bool MappingEngine::addHandler (const ControllerDevice::Control& control,
                                const Node& node,
                                const int parameter)
{
    if (! control.isValid() || ! node.isValid())
        return false;
    auto* const object = node.getObject();
    if (nullptr == object)
        return false;
    if (object->containsParameter (parameter))
    {
        if (auto* input = inputs->findInput (control.getControllerDevice()))
        {
            const auto message (control.getMidiMessage());
            std::unique_ptr<ControllerMapHandler> handler;

            if (message.isController())
                handler.reset (new MidiCCControllerMapHandler (control, message, node, parameter));
            else if (message.isNoteOn())
                handler.reset (new MidiNoteControllerMap (control, message, node, parameter));

            if (nullptr != handler)
            {
                input->addHandler (handler.release());
                return true;
            }
        }
    }
    return false;
}

bool MappingEngine::removeInput (const ControllerDevice& controller)
{
    if (! inputs->containsInputFor (controller))
        return true;
    return inputs->remove (controller);
}

bool MappingEngine::refreshInput (const ControllerDevice& device)
{
    if (! inputs)
        return false;

    if (auto* const input = inputs->findInput (device))
    {
        input->close();
        if (inputs->isRunning())
            input->start();
    }

    return true;
}

void MappingEngine::clear()
{
    stopMapping();
    inputs->clear();
}

void MappingEngine::startMapping()
{
    stopMapping();
    inputs->start();
}

void MappingEngine::stopMapping()
{
    inputs->stop();
}

bool MappingEngine::captureNextEvent (ControllerMapInput& input,
                                      const ControllerDevice::Control& control,
                                      const MidiMessage& message)
{
    if (! capturedEvent.capture.get())
        return false;
    capture (false);
    capturedEvent.cancelPendingUpdate();
    capturedEvent.control = control;
    capturedEvent.message = message;
    capturedEvent.triggerAsyncUpdate();
    return true;
}

} // namespace element
