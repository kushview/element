
#include "engine/MappingEngine.h"
#include "engine/GraphNode.h"
#include "session/ControllerDevice.h"
#include "session/Node.h"

#ifndef EL_MIDI_MAPPING_CHANNELS
 #define EL_MIDI_MAPPING_CHANNELS   0
#endif

namespace Element {

class ControllerMapHandler
{
public:
    ControllerMapHandler() { }
    virtual ~ControllerMapHandler() { }

    virtual bool wants (const MidiMessage& message) const =0;
    virtual void perform (const MidiMessage& message) =0;
};

struct MidiNoteControllerMap : public ControllerMapHandler,
                               public AsyncUpdater,
                               private Value::Listener
{
    MidiNoteControllerMap (const ControllerDevice::Control& ctl,
                           const MidiMessage& message, const Node& _node, 
                           const int _parameter)
        : control (ctl),
          model (_node), node (_node.getGraphNode()),
          processor (node != nullptr ? node->getAudioProcessor() : nullptr),
          noteNumber (message.getNoteNumber()),
          parameterIndex (_parameter),
          parameter (nullptr)
    {
        jassert (message.isNoteOn());
        jassert (node && processor);

       #if EL_MIDI_MAPPING_CHANNELS
        channelObject = control.getPropertyAsValue (Tags::midiChannel);
        channelObject.addListener (this);
        valueChanged (channelObject);
       #endif

        momentaryObject = control.getMomentaryValue();
        momentaryObject.addListener (this);
        valueChanged (momentaryObject);

        inverseObject = control.getInverseToggleObject();
        inverseObject.addListener (this);
        valueChanged (inverseObject);

        if (isPositiveAndBelow (parameterIndex, processor->getParameters().size()))
        {
            parameter = processor->getParameters()[parameterIndex];
            jassert (nullptr != parameter);
        }
    }

    ~MidiNoteControllerMap()
    {
        channelObject.removeListener (this);
    }
    
    bool checkNoteAndChannel (const MidiMessage& message) const
    {
        return message.getNoteNumber() == noteNumber &&
            (channel.get() == 0 || (channel.get() > 0 && message.getChannel() == channel.get()));
    }

    bool wants (const MidiMessage& message) const override
    {
        bool wants = momentary.get() == 0
            ? message.isNoteOn() && checkNoteAndChannel (message)
            : message.isNoteOnOrOff() && checkNoteAndChannel (message);
        // DBG("momentary: " << momentary.get() << " wants: " << (int) wants << " on: " << (int) message.isNoteOn() << " off: " << (int) message.isNoteOff());
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
        else if (parameterIndex == GraphNode::EnabledParameter ||
                 parameterIndex == GraphNode::BypassParameter ||
                 parameterIndex == GraphNode::MuteParameter)
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
            if (parameterIndex == GraphNode::EnabledParameter)
            {
                node->setEnabled (! node->isEnabled());
                model.setProperty (Tags::enabled, node->isEnabled());
            }
            else if (parameterIndex == GraphNode::BypassParameter)
            {
                node->suspendProcessing (! node->isSuspended());
                model.setProperty (Tags::bypass, node->isSuspended());
            }
            else if (parameterIndex == GraphNode::MuteParameter)
            {
                model.setMuted (! model.isMuted());
            }
        }
        else
        {
            jassert (event.isNoteOnOrOff());
            // DBG("async note off: " << (int) event.isNoteOff());
            const bool isInverse = inverse.get() == 1;

            if (parameterIndex == GraphNode::EnabledParameter)
            {
                node->setEnabled (isInverse ? event.isNoteOff() : event.isNoteOn());
                model.setProperty (Tags::enabled, node->isEnabled());
            }
            else if (parameterIndex == GraphNode::BypassParameter)
            {
                node->suspendProcessing (isInverse ? event.isNoteOn() : event.isNoteOff());
                model.setProperty (Tags::bypass, node->isSuspended());
            }
            else if (parameterIndex == GraphNode::MuteParameter)
            {
                model.setMuted (isInverse ? event.isNoteOff() : event.isNoteOn());
            }
        }
    }

private:
    ControllerDevice::Control control;
    Node model;
    GraphNodePtr node;
    AudioProcessor* processor;
    
    Value channelObject;
    Atomic<int> channel { 0 };

    Value momentaryObject;
    Atomic<int> momentary { 0 };

    Value inverseObject;
    Atomic<int> inverse { 0 };

    int parameterIndex = -1;
    AudioProcessorParameter* parameter;
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
        : control (ctl), model (_node), node (_node.getGraphNode()),
          processor (node != nullptr ? node->getAudioProcessor() : nullptr),
          controllerNumber (message.getControllerNumber()),
          parameterIndex (_parameter),
          parameter (nullptr)
    {
        jassert (message.isController());
        jassert (node && processor);

        toggleValueObject = control.getToggleValueObject();
        toggleValueObject.addListener (this);
        toggleValue.set (jlimit (0, 127, control.getToggleValue()));

        inverseToggleObject = control.getInverseToggleObject();
        inverseToggleObject.addListener (this);
        inverseToggle.set (control.inverseToggle() ? 1 : 0);

        toggleModeObject = control.getToggleModeObject();
        toggleModeObject.addListener (this);
        toggleMode.set (static_cast<int> (control.getToggleMode()));

       #if EL_MIDI_MAPPING_CHANNELS
        channelObject = control.getPropertyAsValue (Tags::midiChannel);
        channelObject.addListener (this);
        valueChanged (channelObject);
       #endif

        if (isPositiveAndBelow (parameterIndex, processor->getParameters().size()))
        {
            parameter = processor->getParameters()[parameterIndex];
            jassert (nullptr != parameter);
        }
        else if (parameterIndex == -2)
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
        return message.isController() && 
            message.getControllerNumber() == controllerNumber &&
            (channel.get() == 0 || (channel.get() > 0 && message.getChannel() == channel.get()));
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
        else if (parameterIndex == GraphNode::EnabledParameter ||
                 parameterIndex == GraphNode::BypassParameter ||
                 parameterIndex == GraphNode::MuteParameter)
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
            : 1;                                 // equals mode always compare true

        if (parameterIndex == GraphNode::EnabledParameter)
        {
            node->setEnabled (desiredToggleState.get() == stateToCompare);
            if (model.isEnabled() != node->isEnabled())
                model.setProperty (Tags::enabled, node->isEnabled());
        }
        else if (parameterIndex == GraphNode::BypassParameter)
        {
            // inverted because UI displays bypass as inactive (or active for not bypassed)
            node->suspendProcessing (! (desiredToggleState.get() == stateToCompare));
            if (model.isBypassed() != node->isSuspended())
                model.setProperty (Tags::bypass, node->isSuspended());
        }
        else if (parameterIndex == GraphNode::MuteParameter)
        {
            model.setMuted (desiredToggleState.get() == stateToCompare);
        }
    }

private:
    ControllerDevice::Control control;
    Node model;
    GraphNodePtr node;
    AudioProcessor* processor;
    AudioProcessorParameter* parameter;
    
    const int controllerNumber;
    const int parameterIndex;
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
            inverseToggle.set ((bool)value.getValue() ? 1 : 0);
        }
        else if (toggleModeObject.refersToSameSourceAs (value))
        {
            toggleMode. set (static_cast<int> (ControllerDevice::Control::getToggleMode (
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
    explicit ControllerMapInput (MappingEngine& owner, const ControllerDevice& device)
        : mapping (owner), controllerDevice (device)
    {

    }
    
    ~ControllerMapInput()
    {
        close();
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message)
    {
        if ((! message.isController() || !controllerNumbers [message.getControllerNumber()]) &&
            (!message.isNoteOnOrOff() || !noteNumbers [message.getNoteNumber()]))
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
        if (midiInput != nullptr)
        {
            stop();
            midiInput.reset (nullptr);
        }

        controllerNumbers.setRange (0, 127, false);
        noteNumbers.setRange (0, 127, false);
        return midiInput == nullptr;
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

        if (midiInput == nullptr)
        {
            const auto devices = MidiInput::getDevices();
            const auto index = devices.indexOf (controllerDevice.getInputDevice().toString());
            if (isPositiveAndBelow (index, devices.size()))
                midiInput.reset (MidiInput::openDevice (index, this));
        }

        return midiInput != nullptr;
    }

    void start()
    {
        if (midiInput == nullptr)
            open();
        if (midiInput)
            midiInput->start();
    }

    void stop()
    {
        if (midiInput)
            midiInput->stop();
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
    MappingEngine& mapping;
    ControllerDevice controllerDevice;
    std::unique_ptr<MidiInput> midiInput;
    OwnedArray<ControllerMapHandler> handlers;
    BigInteger controllerNumbers, noteNumbers;
    HashMap<int, ControllerDevice::Control> controls, notes;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerMapInput)
};

class MappingEngine::Inputs
{
public:
    Inputs() { }
    ~Inputs() { }

    bool add (ControllerMapInput* input)
    {
        inputs.addIfNotAlreadyThere (input);
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

    ControllerMapInput** begin()  const noexcept { return inputs.begin(); }
    ControllerMapInput** end()    const noexcept { return inputs.end(); }
    int size()                    const noexcept { return inputs.size(); }

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

bool MappingEngine::addInput (const ControllerDevice& controller)
{
    if (inputs->containsInputFor (controller))
        return true;
    
    std::unique_ptr<ControllerMapInput> input;
    input.reset (new ControllerMapInput (*this, controller));

    // DBG("[EL] added input for " << controller.getName().toString());
    return inputs->add (input.release());
}

bool MappingEngine::addHandler (const ControllerDevice::Control& control, 
                                const Node& node, const int parameter)
{
    if (! control.isValid() || ! node.isValid())
        return false;
    auto* const object = node.getGraphNode();
    auto* const proc   = object == nullptr ? nullptr : object->getAudioProcessor();
    if (nullptr == object || nullptr == proc)
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
    if (! inputs) return false;

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

}
