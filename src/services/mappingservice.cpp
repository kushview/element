// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "services/mappingservice.hpp"
#include <element/ui.hpp>
#include "engine/mappingengine.hpp"
#include <element/context.hpp>
#include <element/signals.hpp>

using namespace juce;

namespace element {

enum LearnState
{
    CaptureStopped = 0,
    CaptureParameter,
    CaptureControl
};

class AudioProcessorParameterCapture : public AsyncUpdater
{
public:
    AudioProcessorParameterCapture()
    {
        capture.set (false);
    }

    ~AudioProcessorParameterCapture() noexcept
    {
        clear();
    }

    void handleAsyncUpdate() override
    {
        AudioProcessor* capturedProcessor = nullptr;
        ProcessorPtr capturedObject = nullptr;
        Node capturedNode = Node();
        int capturedParameter = Processor::NoParameter;

        {
            ScopedLock sl (lock);
            capturedNode = node;
            capturedObject = object;
            capturedProcessor = processor;
            capturedParameter = parameter;
            ignoreUnused (capturedProcessor);

            node = Node();
            object = nullptr;
            processor = nullptr;
            parameter = Processor::NoParameter;
        }

        if (capturedObject != nullptr && capturedObject.get() == capturedNode.getObject())
        {
            if (Processor::isSpecialParameter (capturedParameter) || isPositiveAndBelow (capturedParameter, capturedObject->getParameters().size()))
            {
                callback (capturedNode, capturedParameter);
            }
        }

        clear();
    }

    void clear()
    {
        capture.set (false);
        for (auto* mappable : mappables)
            mappable->clear();
        mappables.clearQuick (true);
    }

    void addNodes (SessionPtr session)
    {
        clear();

        capture.set (false);

        for (int i = 0; i < session->getNumGraphs(); ++i)
        {
            const auto graph (session->getGraph (i));
            addNodesRecursive (graph);
        }

        capture.set (true);
    }

    CriticalSection lock;
    Signal<void (const Node&, int)> callback;
    Atomic<bool> capture = false;
    Node node;
    ProcessorPtr object = nullptr;
    AudioProcessor* processor = nullptr;
    int parameter = -1;

private:
    class Mappable : public Parameter::Listener
    {
    public:
        Mappable (AudioProcessorParameterCapture& c, const Node& n)
            : capture (c), node (n), object (n.getObject())
        {
            connect();
        }

        ~Mappable() noexcept
        {
            clear();
        }

        void clear()
        {
            for (auto* const param : object->getParameters())
                param->removeListener (this);
            for (auto& c : connections)
                c.disconnect();
        }

        void connect()
        {
            if (connections.size() > 0)
                clear();

            connections.add (object->enablementChanged.connect (std::bind (
                &Mappable::onEnablementChanged, this, std::placeholders::_1)));
            connections.add (object->bypassChanged.connect (std::bind (
                &Mappable::onBypassChanged, this, std::placeholders::_1)));
            connections.add (object->muteChanged.connect (std::bind (
                &Mappable::onMuteChanged, this, std::placeholders::_1)));
            connections.add (object->gainChanged.connect (std::bind (
                &Mappable::onGainChanged, this, std::placeholders::_1)));
            connections.add (object->inputGainChanged.connect (std::bind (
                &Mappable::onInputGainChanged, this, std::placeholders::_1)));

            for (auto* const param : object->getParameters())
                param->addListener (this);
        }

        void controlValueChanged (int index, float) override
        {
            if (capture.capture.get() == false)
                return;
            ScopedLock sl (capture.lock);
            capture.capture.set (false);
            capture.node = node;
            capture.object = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = index;
            capture.triggerAsyncUpdate();
        }

        void controlTouched (int, bool) override {}

        void onEnablementChanged (Processor*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node = node;
            capture.object = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = Processor::EnabledParameter;
            capture.triggerAsyncUpdate();
        }

        void onBypassChanged (Processor*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node = node;
            capture.object = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = Processor::BypassParameter;
            capture.triggerAsyncUpdate();
        }

        void onMuteChanged (Processor*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node = node;
            capture.object = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = Processor::MuteParameter;
            capture.triggerAsyncUpdate();
        }

        void onGainChanged (Processor*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node = node;
            capture.object = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = Processor::OutputGainParameter;
            capture.triggerAsyncUpdate();
        }

        void onInputGainChanged (Processor*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node = node;
            capture.object = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = Processor::InputGainParameter;
            capture.triggerAsyncUpdate();
        }

    private:
        AudioProcessorParameterCapture& capture;
        Node node;
        ProcessorPtr object;
        Array<SignalConnection> connections;
    };

    OwnedArray<Mappable> mappables;

    void addNodesRecursive (const Node& node)
    {
        for (int j = 0; j < node.getNumNodes(); ++j)
        {
            const auto child (node.getNode (j));
            if (Processor* const object = child.getObject())
                mappables.add (new Mappable (*this, child));
            if (child.data().getNumChildren() > 0)
                addNodesRecursive (child);
        }
    }
};

class MappingService::Impl
{
public:
    Impl() {}
    ~Impl()
    {
        capture.clear();
    }

    bool isCaptureComplete() const
    {
        ProcessorPtr object = node.getObject();
        return object != nullptr && (Processor::isSpecialParameter (parameter) || isPositiveAndBelow (parameter, object->getParameters().size())) && (message.isController() || message.isNoteOnOrOff());
    }

    AudioProcessorParameterCapture capture;
    LearnState learnState = CaptureStopped;

    Node node = Node();
    int parameter = -1;
    MidiMessage message;
    String device;

    // Target for the armed capture: "parameter" (node + param) or "tempo".
    String pendingTargetType = "parameter";
};

MappingService::MappingService()
{
    impl.reset (new Impl());
}

MappingService::~MappingService()
{
    capturedConnection.disconnect();
    capturedParamConnection.disconnect();
    impl = nullptr;
}

void MappingService::activate()
{
    Service::activate();
    auto& capture (impl->capture);
    auto& mapping (context().mapping());
    capturedConnection = mapping.mappingCapturedSignal().connect (
        std::bind (&MappingService::onControlCaptured, this));
    capturedParamConnection = capture.callback.connect (
        std::bind (&MappingService::onParameterCaptured, this, std::placeholders::_1, std::placeholders::_2));
    mapping.rebuildBindings (context().session());
    mapping.startListening (context().midi());
}

void MappingService::deactivate()
{
    Service::deactivate();
    context().mapping().stopListening (context().midi());
    capturedConnection.disconnect();
    capturedParamConnection.disconnect();
}

bool MappingService::isLearning() const
{
    return impl && impl->learnState != CaptureStopped;
}

void MappingService::learn (const bool shouldLearn)
{
    auto& capture (impl->capture);
    auto& mapping (context().mapping());

    impl->learnState = CaptureStopped;
    impl->pendingTargetType = "parameter";
    capture.clear();
    mapping.captureMapping (false);

    if (shouldLearn)
    {
        DBG ("[element] MappingService: start learning");
        impl->learnState = CaptureParameter;
        capture.addNodes (context().session());
    }
}

void MappingService::tapTempo()
{
    if (auto bpm = context().mapping().tapTempo (Time::getMillisecondCounterHiRes()))
        if (auto session = context().session())
            session->data().setProperty (tags::tempo, *bpm, nullptr);
}

void MappingService::learnTempo()
{
    auto& mapping (context().mapping());

    // Skip the parameter-capture phase: arm MIDI capture directly and bind the
    // next event to the session tempo.
    impl->capture.clear();
    impl->pendingTargetType = "tempo";
    impl->learnState = CaptureControl;
    mapping.captureMapping (true);
}

bool MappingService::hasTempoMapping()
{
    auto session = context().session();
    if (session == nullptr)
        return false;
    for (int i = 0; i < session->getNumMidiMappings(); ++i)
        if (session->getMidiMapping (i).isTempoTarget())
            return true;
    return false;
}

String MappingService::getTempoMappingDescription()
{
    auto session = context().session();
    if (session == nullptr)
        return {};
    for (int i = 0; i < session->getNumMidiMappings(); ++i)
    {
        auto m = session->getMidiMapping (i);
        if (m.isTempoTarget())
            return (m.isNoteEvent() ? String ("Note ") : String ("CC ")) + String (m.getEventId());
    }
    return {};
}

void MappingService::clearTempoMapping()
{
    auto session = context().session();
    if (session == nullptr)
        return;

    bool removed = false;
    for (int i = session->getNumMidiMappings(); --i >= 0;)
    {
        auto m = session->getMidiMapping (i);
        if (m.isTempoTarget())
        {
            session->removeMidiMapping (m);
            removed = true;
        }
    }

    if (removed)
    {
        context().mapping().rebuildBindings (session);
        if (auto* gui = sibling<GuiService>())
            gui->stabilizeViews();
    }
}

void MappingService::onParameterCaptured (const Node& node, int parameter)
{
    if (impl->learnState == CaptureParameter)
    {
        DBG ("[element] MappingService: got parameter: " << parameter);
        auto& mapping (context().mapping());
        impl->learnState = CaptureControl;
        impl->node = node;
        impl->parameter = parameter;
        mapping.captureMapping (true);
    }
    else
    {
        DBG ("[element] received captured param: invalid state: " << (int) impl->learnState);
    }
}

void MappingService::onControlCaptured()
{
    auto session = context().session();

    if (impl->learnState == CaptureControl)
    {
        auto& mapping (context().mapping());
        impl->learnState = CaptureStopped;
        impl->message = mapping.getCapturedMessage();
        impl->device = mapping.getCapturedDevice();

        DBG ("[element] MappingService: captured MIDI on device: " << impl->device);

        const bool haveMidi = impl->message.isController() || impl->message.isNoteOnOrOff();

        if (impl->pendingTargetType == "tempo" && haveMidi)
        {
            session->addMidiMapping (MidiMapping::fromCaptureTempo (impl->device, impl->message));
            mapping.rebuildBindings (session); // live immediately

            if (auto* gui = sibling<GuiService>())
                gui->stabilizeViews();
        }
        else if (impl->isCaptureComplete())
        {
            auto newMapping = MidiMapping::fromCapture (
                impl->device, impl->message, "parameter", impl->node.getUuid(), impl->parameter);
            session->addMidiMapping (newMapping);
            mapping.rebuildBindings (session); // live immediately

            if (auto* gui = sibling<GuiService>())
                gui->stabilizeViews();
        }

        impl->pendingTargetType = "parameter";
    }
    else
    {
        DBG ("[element] received captured control: invalid state: " << (int) impl->learnState);
    }
}

void MappingService::refresh()
{
    context().mapping().rebuildBindings (context().session());
}

void MappingService::remove (const MidiMapping& mapping)
{
    auto session = context().session();
    session->removeMidiMapping (mapping);
    context().mapping().rebuildBindings (session);
    if (auto* gui = sibling<GuiService>())
        gui->stabilizeViews();
}

} // namespace element
