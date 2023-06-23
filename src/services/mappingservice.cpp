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

#include "services/mappingservice.hpp"
#include "services/deviceservice.hpp"
#include <element/ui.hpp>
#include "engine/mappingengine.hpp"
#include <element/controller.hpp>
#include <element/context.hpp>
#include <element/signals.hpp>

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
            if (capturedParameter == Processor::EnabledParameter || capturedParameter == Processor::BypassParameter || capturedParameter == Processor::MuteParameter || isPositiveAndBelow (capturedParameter, capturedObject->getParameters().size()))
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
        return object != nullptr && (parameter == Processor::EnabledParameter || parameter == Processor::BypassParameter || parameter == Processor::MuteParameter || isPositiveAndBelow (parameter, object->getParameters().size())) && (message.isController() || message.isNoteOn()) && control.data().isValid();
    }

    AudioProcessorParameterCapture capture;
    LearnState learnState = CaptureStopped;

    Node node = Node();
    int parameter = -1;
    MidiMessage message;
    Control control;
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
    capturedConnection = context().mapping().capturedSignal().connect (
        std::bind (&MappingService::onControlCaptured, this));
    capturedParamConnection = capture.callback.connect (
        std::bind (&MappingService::onParameterCaptured, this, std::placeholders::_1, std::placeholders::_2));
    context().mapping().startMapping();
}

void MappingService::deactivate()
{
    Service::deactivate();
    context().mapping().stopMapping();
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
    capture.clear();
    mapping.capture (false);

    if (shouldLearn)
    {
        DBG ("[element] MappingService: start learning");
        impl->learnState = CaptureParameter;
        capture.addNodes (context().session());
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
        mapping.capture (true);
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
        impl->message = mapping.getCapturedMidiMessage();
        impl->control = mapping.getCapturedControl();

        DBG ("[element] MappingService: got control: " << impl->control.getName().toString());

        if (impl->isCaptureComplete())
        {
            if (mapping.addHandler (impl->control, impl->node, impl->parameter))
            {
                ValueTree newMap (tags::map);
                newMap.setProperty (tags::controller, impl->control.controller().getUuidString(), nullptr)
                    .setProperty (tags::control, impl->control.getUuidString(), nullptr)
                    .setProperty (tags::node, impl->node.getUuidString(), nullptr)
                    .setProperty (tags::parameter, impl->parameter, nullptr);
                auto maps = session->data().getChildWithName (tags::maps);
                maps.addChild (newMap, -1, nullptr);

                if (auto* gui = sibling<GuiService>())
                    gui->stabilizeViews();
            }
        }
    }
    else
    {
        DBG ("[element] received captured control: invalid state: " << (int) impl->learnState);
    }
}

void MappingService::remove (const ControllerMap& controllerMap)
{
    auto session = context().session();
    auto maps = session->data().getChildWithName (tags::maps);
    if (controllerMap.data().isAChildOf (maps))
    {
        maps.removeChild (controllerMap.data(), nullptr);
        if (auto* devs = sibling<DeviceService>())
            devs->refresh();
    }
}

} // namespace element
