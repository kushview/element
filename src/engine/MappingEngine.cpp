
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
        jassert (message.isNoteOnOrOff());
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
    explicit ControllerMapInput (const ControllerDevice& device)
        : controllerDevice (device) 
    { }
    
    ~ControllerMapInput()
    {
        close();
    }

    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
    {
        DBG("[EL] midi in controller map input: " << source->getName());
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

        return midiInput == nullptr;
    }

    bool open()
    {
        close();

        if (midiInput == nullptr)
        {
            const auto devices = MidiInput::getDevices();
            const auto index = devices.indexOf (controllerDevice.getName().toString());
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

private:
    ControllerDevice controllerDevice;
    std::unique_ptr<MidiInput> midiInput;
    OwnedArray<ControllerMapHandler> handlers;
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
        input->start();
        return inputs.contains (input);
    }

    bool remove (const ControllerDevice& device)
    {
        if (auto* input = findInput (device))
            inputs.removeObject (input, true);
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

private:
    OwnedArray<ControllerMapInput> inputs;
    bool running = false;
};

MappingEngine::MappingEngine()
{ 
    inputs.reset (new Inputs());
}

MappingEngine::~MappingEngine()
{
    inputs->clear();
    inputs = nullptr;
}

bool MappingEngine::addInput (const ControllerDevice& controller)
{
    DBG("[EL] MappingEngine::addInput(...)");
    if (inputs->containsInputFor (controller))
        return true;
    return inputs->add (new ControllerMapInput (controller));
}

bool MappingEngine::removeInput (const ControllerDevice& controller)
{
    DBG("[EL] MappingEngine::removeInput(...)");
    if (! inputs->containsInputFor (controller))
        return true;
    return inputs->remove (controller);
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

}
