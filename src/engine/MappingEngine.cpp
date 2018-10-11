
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
    ControllerMapInput (const String& name)
        : deviceName (name) 
    { 

    }
    
    ~ControllerMapInput()
    {
        close();
        deviceName.clear();
    }

    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
    {
        DBG("midi in controller map input: " << source->getName());
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
            const auto index = devices.indexOf (deviceName);
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

private:
    String deviceName;
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

    void clear()
    {
        for (auto* input : inputs)
        {
            input->stop();
            input->close();
        }

        inputs.clear (true);
    }

    void start()
    {
        for (auto* input : inputs)
            input->start();
    }

    void stop()
    {
        for (auto* input : inputs)
            input->stop();
    }

private:
    OwnedArray<ControllerMapInput> inputs;
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
    const auto devices (MidiInput::getDevices());
    const auto inputDevice (controller.getInputDevice().toString());
    if (isPositiveAndBelow (devices.indexOf (inputDevice), devices.size()))
        return inputs->add (new ControllerMapInput (inputDevice));
    return false;
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
