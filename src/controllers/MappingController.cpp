
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
        int capturedParameter = -1;

        {
            ScopedLock sl (lock);
            capturedProcessor = processor;
            capturedParameter = parameter;
            processor = nullptr;
            parameter = -1;
        }

        const auto captured (nodeMap2 [capturedProcessor]);
        if (auto* node = captured.getGraphNode())
            if (auto* proc = node->getAudioProcessor())
                if (proc == capturedProcessor && isPositiveAndBelow (capturedParameter, proc->getParameters().size()))
                    callback (captured, capturedParameter);

        clear();
    }

    void audioProcessorChanged (AudioProcessor*) override { }

    void clear()
    {
        capture.set (false);

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
            for (int j = 0; j < graph.getNumNodes(); ++j)
            {
                const auto node (graph.getNode (j));
                if (GraphNodePtr object = node.getGraphNode())
                    if (auto* proc = object->getAudioProcessor())
                        { 
                            nodeMap2.set (proc, node);
                            proc->addListener (this); 
                            DBG("[EL] added listener: " << proc->getName());
                        }
            }
        }

        capture.set (true);
    }

    CriticalSection lock;
    boost::signals2::signal<void(const Node&, int)> callback;
    Atomic<bool> capture        = false;
    AudioProcessor* processor   = nullptr;
    int parameter               = -1;
    HashMap<AudioProcessor*, Node> nodeMap2;
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

        return object && proc && isPositiveAndBelow (parameter, proc->getParameters().size()) &&
            message.isController() && control.getValueTree().isValid();
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
    auto& capture (impl->capture);
    capturedConnection = getWorld().getMappingEngine().capturedSignal().connect (
        std::bind (&MappingController::onControlCaptured, this));
    capturedParamConnection = capture.callback.connect (
        std::bind (&MappingController::onParameterCaptured, this, 
            std::placeholders::_1, std::placeholders::_2));
    getWorld().getMappingEngine().startMapping();
}

void MappingController::deactivate() 
{
    Controller::deactivate();
    getWorld().getMappingEngine().stopMapping();
    capturedConnection.disconnect();
    capturedParamConnection.disconnect();
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

    if (shouldLearn)
    {
        impl->learnState = CaptureParameter;
        capture.addNodes (getWorld().getSession());
    } 
}

void MappingController::onParameterCaptured (const Node& node, int parameter)
{
    if (impl->learnState == CaptureParameter)
    {
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
    auto session = getWorld().getSession();

    if (impl->learnState == CaptureControl)
    {
        auto& mapping (getWorld().getMappingEngine());
        impl->learnState = CaptureStopped;
        impl->message = mapping.getCapturedMidiMessage();
        impl->control = mapping.getCapturedControl();
        if (impl->isCaptureComplete())
        {
            if (mapping.addHandler (impl->control, impl->node, impl->parameter))
            {
                ValueTree newMap (Tags::map);
                newMap.setProperty (Tags::controller,   impl->control.getControllerDevice().getUuidString(), nullptr)
                      .setProperty (Tags::control,      impl->control.getUuidString(), nullptr)
                      .setProperty (Tags::node,         impl->node.getUuidString(), nullptr)
                      .setProperty (Tags::parameter,    impl->parameter, nullptr);
                auto maps = session->getValueTree().getChildWithName(Tags::maps);
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
