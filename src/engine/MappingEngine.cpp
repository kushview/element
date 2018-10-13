
#include "engine/MappingEngine.h"
#include "engine/GraphNode.h"
#include "session/ControllerDevice.h"
#include "session/Node.h"

namespace Element {

struct ControllerMapHandler
{
    ControllerMapHandler() { }
    virtual ~ControllerMapHandler() { }

    virtual bool wants (const MidiMessage& message) const =0;
    virtual void perform (const MidiMessage& message) =0;
};

struct MidiNoteControllerMap : public ControllerMapHandler
{
    MidiNoteControllerMap (const MidiMessage& message, const Node& _node, const int _parameter)
        : node (_node.getGraphNode()),
          processor (node != nullptr ? node->getAudioProcessor() : nullptr),
          noteNumber (message.getNoteNumber()),
          parameter (processor->getParameters()[_parameter])
    {
        jassert (message.isNoteOnOrOff());
        jassert (node && processor && parameter);
    }

    bool wants (const MidiMessage& message) const override
    {
        return message.isNoteOnOrOff() && message.getNoteNumber() == noteNumber;
    }

    void perform (const MidiMessage&) override
    {
        parameter->setValue (parameter->getValue() < 0.5 ? 1.f : 0.f);
    }

private:
    GraphNodePtr node;
    AudioProcessor* processor;
    AudioProcessorParameter* parameter;
    const int noteNumber;
};

struct MidiCCControllerMapHandler : public ControllerMapHandler
{
    MidiCCControllerMapHandler (const MidiMessage& message, const Node& _node, const int _parameter)
        : node (_node.getGraphNode()),
          processor (node != nullptr ? node->getAudioProcessor() : nullptr),
          controllerNumber (message.getControllerNumber()),
          parameter (processor->getParameters()[_parameter])
    {
        jassert (message.isController());
        jassert (node && processor && parameter);
    }

    bool wants (const MidiMessage& message) const override
    {
        return message.isController() && 
            message.getControllerNumber() == controllerNumber;
    }

    void perform (const MidiMessage& message) override
    {
        parameter->setValue (static_cast<float> (message.getControllerValue()) / 127.f);
    }

private:
    GraphNodePtr node;
    AudioProcessor* processor;
    AudioProcessorParameter* parameter;
    const int controllerNumber;
};

class ControllerMapInput : public MidiInputCallback
{
public:
    explicit ControllerMapInput (MappingEngine& owner, const ControllerDevice& device)
        : mapping(owner), controllerDevice (device) 
    {

    }
    
    ~ControllerMapInput()
    {
        close();
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message)
    {
        if (! message.isController() || ! controllerNumbers [message.getControllerNumber()])
            return;

        // DBG("[EL] handle mapped MIDI: " << message.getControllerNumber() 
        //     << " : " << message.getControllerValue());
        mapping.captureNextEvent (*this, controls [message.getControllerNumber()], message);

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
        return midiInput == nullptr;
    }

    bool open()
    {
        close();

        for (int i = controllerDevice.getNumControls(); --i >= 0;)
        {
            const auto control (controllerDevice.getControl (i));
            const auto midi (control.getMappingData());
            if (! midi.isController())
                continue;
            controllerNumbers.setBit (midi.getControllerNumber(), true);
            controls.set (midi.getControllerNumber(), control);
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
    BigInteger controllerNumbers;
    HashMap<int, ControllerDevice::Control> controls;
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

    DBG("[EL] added input for " << controller.getName().toString());
    return inputs->add (input.release());
}

bool MappingEngine::addHandler (const ControllerDevice::Control& control, 
                                const Node& node, const int parameter)
{
    if (auto* input = inputs->findInput (control.getControllerDevice()))
    {
        const auto message (control.getMappingData());
        std::unique_ptr<MidiCCControllerMapHandler> handler;
        handler.reset (new MidiCCControllerMapHandler (message, node, parameter));
        input->addHandler (handler.release());
        return true;
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
