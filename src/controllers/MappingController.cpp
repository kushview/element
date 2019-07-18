
#include "controllers/MappingController.h"
#include "controllers/DevicesController.h"
#include "controllers/GuiController.h"
#include "engine/MappingEngine.h"
#include "session/ControllerDevice.h"
#include "Globals.h"
#include "Signals.h"

namespace Element {

enum LearnState
{
    CaptureStopped = 0,
    CaptureParameter,
    CaptureControl
};

class AudioProcessorParameterCapture : public AudioProcessorListener,
                                       public AsyncUpdater
{
public:
    AudioProcessorParameterCapture() { capture.set (false); }
    ~AudioProcessorParameterCapture() noexcept
    {
        clear();
    }

    void audioProcessorParameterChanged (AudioProcessor* processorThatChanged,
                                         int parameterIndex,
                                         float newValue) override
    {
        if (capture.get() == false)
            return;
        capture.set (false);
        ScopedLock sl (lock);
        processor = processorThatChanged;
        parameter = parameterIndex;
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override 
    {
        AudioProcessor* capturedProcessor = nullptr;
        int capturedParameter = GraphNode::NoParameter;

        {
            ScopedLock sl (lock);
            capturedProcessor = processor;
            capturedParameter = parameter;
            processor = nullptr;
            parameter = GraphNode::NoParameter;
        }

        const auto captured (nodeMap2 [capturedProcessor]);
        if (auto* node = captured.getGraphNode())
        {
            if (auto* proc = node->getAudioProcessor())
            {
                if (proc == capturedProcessor && 
                        (capturedParameter == GraphNode::EnabledParameter || 
                         capturedParameter == GraphNode::BypassParameter ||
                         capturedParameter == GraphNode::MuteParameter ||
                         isPositiveAndBelow (capturedParameter, proc->getParameters().size())))
                {
                    callback (captured, capturedParameter);
                }
            }
        }

        clear();
    }

    void audioProcessorChanged (AudioProcessor*) override { }

    void clear()
    {
        capture.set (false);

        for (auto& c : nodeConnections)
            c.disconnect();
        nodeConnections.clear();

        for (const auto& item : nodeMap2)
            if (auto* node = item.getGraphNode())
                if (auto* proc = node->getAudioProcessor())
                    proc->removeListener (this);

        nodeMap2.clear();
    }

    void addNodes (SessionPtr session)
    {
        clear();

        for (int i = 0; i < session->getNumGraphs(); ++i)
        {
            const auto graph (session->getGraph (i));
            addNodesRecursive (graph);
        }

        capture.set (true);
    }

    CriticalSection lock;
    Signal<void(const Node&, int)> callback;
    Atomic<bool> capture        = false;
    AudioProcessor* processor   = nullptr;
    int parameter               = -1;
    HashMap<AudioProcessor*, Node> nodeMap2;

private:
    Array<SignalConnection> nodeConnections;

    void onEnablementChanged (GraphNode* ptr)
    {
        if (capture.get() == false)
            return;
        capture.set (false);
        ScopedLock sl (lock);
        processor = ptr != nullptr ? ptr->getAudioProcessor() : nullptr;
        parameter = GraphNode::EnabledParameter;
        triggerAsyncUpdate();
    }

    void onBypassChanged (GraphNode* ptr)
    {
        GraphNodePtr ref = ptr;
        if (capture.get() == false)
            return;
        capture.set (false);
        ScopedLock sl (lock);
        processor = ptr != nullptr ? ptr->getAudioProcessor() : nullptr;
        parameter = GraphNode::BypassParameter;
        triggerAsyncUpdate();
    }

    void onMuteChanged (GraphNode* ptr)
    {
        GraphNodePtr ref = ptr;
        if (capture.get() == false)
            return;
        capture.set (false);
        ScopedLock sl (lock);
        processor = ptr != nullptr ? ptr->getAudioProcessor() : nullptr;
        parameter = GraphNode::MuteParameter;
        triggerAsyncUpdate();
    }

    void addNodesRecursive (const Node& node)
    {
        for (int j = 0; j < node.getNumNodes(); ++j)
        {
            const auto n (node.getNode (j));
            if (GraphNodePtr object = n.getGraphNode())
            {
                if (auto* proc = object->getAudioProcessor())
                {
                    nodeMap2.set (proc, n);
                    nodeConnections.add (object->enablementChanged.connect (std::bind (
                        &AudioProcessorParameterCapture::onEnablementChanged, this, 
                        std::placeholders::_1)));
                    nodeConnections.add (object->bypassChanged.connect (std::bind (
                        &AudioProcessorParameterCapture::onBypassChanged, this, 
                        std::placeholders::_1)));
                    nodeConnections.add (object->muteChanged.connect (std::bind (
                        &AudioProcessorParameterCapture::onMuteChanged, this, 
                        std::placeholders::_1)));
                    proc->addListener (this);
                }
            }
            if (n.getNumChildren() > 0)
                addNodesRecursive (n);
        }
    }
};

class MappingController::Impl
{
public:
    Impl() { }
    ~Impl()
    {
        capture.clear();
    }

    bool isCaptureComplete() const
    {
        GraphNodePtr object = node.getGraphNode();
        AudioProcessor* proc = (object != nullptr) ? object->getAudioProcessor() : nullptr;
        return object && proc && 
            (parameter == GraphNode::EnabledParameter || 
             parameter == GraphNode::BypassParameter || 
             parameter == GraphNode::MuteParameter ||
             isPositiveAndBelow (parameter, proc->getParameters().size())) &&
            (message.isController() || message.isNoteOn()) && 
            control.getValueTree().isValid();
    }

    AudioProcessorParameterCapture capture;
    LearnState learnState = CaptureStopped;

    Node node = Node();
    int parameter = -1;
    MidiMessage message;
    ControllerDevice::Control control;
};

MappingController::MappingController()
{
    impl.reset (new Impl());
}

MappingController::~MappingController()
{
    capturedConnection.disconnect();
    capturedParamConnection.disconnect();
    impl = nullptr;
}

void MappingController::activate()
{
    Controller::activate();
   #ifndef EL_FREE
    auto& capture (impl->capture);
    capturedConnection = getWorld().getMappingEngine().capturedSignal().connect (
        std::bind (&MappingController::onControlCaptured, this));
    capturedParamConnection = capture.callback.connect (
        std::bind (&MappingController::onParameterCaptured, this, 
            std::placeholders::_1, std::placeholders::_2));
    getWorld().getMappingEngine().startMapping();
   #endif
}

void MappingController::deactivate() 
{
    Controller::deactivate();
   #ifndef EL_FREE
    getWorld().getMappingEngine().stopMapping();
    capturedConnection.disconnect();
    capturedParamConnection.disconnect();
   #endif
}

bool MappingController::isLearning() const
{
    return impl && impl->learnState != CaptureStopped;
}

void MappingController::learn (const bool shouldLearn)
{
    auto& capture (impl->capture);
    auto& mapping (getWorld().getMappingEngine());

    impl->learnState = CaptureStopped;
    capture.clear();
    mapping.capture (false);

    returnIfNotFullVersion

    if (shouldLearn)
    {
        DBG("[EL] MappingController: start learning");
        impl->learnState = CaptureParameter;
        capture.addNodes (getWorld().getSession());
    }
}

void MappingController::onParameterCaptured (const Node& node, int parameter)
{
    returnIfNotFullVersion

    if (impl->learnState == CaptureParameter)
    {
        DBG("[EL] MappingController: got parameter: " << parameter);
        auto& mapping (getWorld().getMappingEngine());
        impl->learnState = CaptureControl;
        impl->node = node;
        impl->parameter = parameter;
        mapping.capture (true);
    }
    else
    {
        DBG("[EL] received captured param: invalid state: " << (int) impl->learnState);
    }
}

void MappingController::onControlCaptured()
{
    returnIfNotFullVersion
    auto session = getWorld().getSession();

    if (impl->learnState == CaptureControl)
    {
        auto& mapping (getWorld().getMappingEngine());
        impl->learnState = CaptureStopped;
        impl->message = mapping.getCapturedMidiMessage();
        impl->control = mapping.getCapturedControl();

        DBG("[EL] MappingController: got control: " << impl->control.getName().toString());

        if (impl->isCaptureComplete())
        {
            if (mapping.addHandler (impl->control, impl->node, impl->parameter))
            {
                ValueTree newMap (Tags::map);
                newMap.setProperty (Tags::controller,   impl->control.getControllerDevice().getUuidString(), nullptr)
                      .setProperty (Tags::control,      impl->control.getUuidString(), nullptr)
                      .setProperty (Tags::node,         impl->node.getUuidString(), nullptr)
                      .setProperty (Tags::parameter,    impl->parameter, nullptr);
                auto maps = session->getValueTree().getChildWithName (Tags::maps);
                maps.addChild (newMap, -1, nullptr);

                if (auto* gui = findSibling<GuiController>())
                    gui->stabilizeContent();
            }
        }
    }
    else
    {
        DBG("[EL] received captured control: invalid state: " << (int) impl->learnState);
    }
}

void MappingController::remove (const ControllerMap& controllerMap)
{
    auto session = getWorld().getSession();
    auto maps = session->getValueTree().getChildWithName(Tags::maps);
    if (controllerMap.getValueTree().isAChildOf (maps))
    {
        maps.removeChild (controllerMap.getValueTree(), nullptr);
        if (auto* devs = findSibling<DevicesController>())
            devs->refresh();
    }
}

}
