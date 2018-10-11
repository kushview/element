
#include "engine/MappingEngine.h"
#include "engine/GraphNode.h"
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
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
    {
        jassert (source == midiInput.get());
        for (auto* handler : handlers)
            if (handler->wants (message))
                handler->perform (message);
    }

    void start()
    {
        if (midiInput)
            midiInput->start();
    }

    void stop()
    {
        if (midiInput)
            midiInput->stop();
    }

private:
    std::unique_ptr<MidiInput> midiInput;
    OwnedArray<ControllerMapHandler> handlers;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControllerMapInput)
};

class MappingEngine::Inputs
{
public:
    Inputs() { }
    ~Inputs() { }

private:
    OwnedArray<ControllerMapInput> inputs;
};

MappingEngine::MappingEngine()
{ 
    inputs.reset (new Inputs());
}

MappingEngine::~MappingEngine()
{ 
    inputs = nullptr;
}

void MappingEngine::startMapping()
{
}

void MappingEngine::stopMapping()
{
}

}
