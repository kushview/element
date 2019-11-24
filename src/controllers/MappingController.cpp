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
        GraphNodePtr capturedObject = nullptr;
        Node capturedNode = Node();
        int capturedParameter = GraphNode::NoParameter;

        {
            ScopedLock sl (lock);
            capturedNode      = node;
            capturedObject    = object;
            capturedProcessor = processor;
            capturedParameter = parameter;

            node        = Node();
            object      = nullptr;
            processor   = nullptr;
            parameter   = GraphNode::NoParameter;
        }

        if (capturedObject != nullptr && capturedObject.get() == capturedNode.getGraphNode())
        {
            if (capturedParameter == GraphNode::EnabledParameter ||
                capturedParameter == GraphNode::BypassParameter ||
                capturedParameter == GraphNode::MuteParameter ||
                isPositiveAndBelow (capturedParameter, capturedObject->getParameters().size()))
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
    Signal<void(const Node&, int)> callback;
    Atomic<bool> capture        = false;
    Node node;
    GraphNodePtr object         = nullptr;
    AudioProcessor* processor   = nullptr;
    int parameter               = -1;

private:
    class Mappable : public Parameter::Listener
    {
    public:
        Mappable (AudioProcessorParameterCapture& c, const Node& n)
            : capture (c), node (n), object (n.getGraphNode())
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
            capture.node      = node;
            capture.object    = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = index;
            capture.triggerAsyncUpdate();
        }

        void controlTouched (int, bool) override {}

        void onEnablementChanged (GraphNode*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node      = node;
            capture.object    = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = GraphNode::EnabledParameter;
            capture.triggerAsyncUpdate();
        }

        void onBypassChanged (GraphNode*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node      = node;
            capture.object    = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = GraphNode::BypassParameter;
            capture.triggerAsyncUpdate();
        }

        void onMuteChanged (GraphNode*)
        {
            if (capture.capture.get() == false)
                return;
            capture.capture.set (false);
            ScopedLock sl (capture.lock);
            capture.node      = node;
            capture.object    = object;
            capture.processor = object->getAudioProcessor();
            capture.parameter = GraphNode::MuteParameter;
            capture.triggerAsyncUpdate();
        }

    private:
        AudioProcessorParameterCapture& capture;
        Node node;
        GraphNodePtr object;
        Array<SignalConnection> connections;
    };

    OwnedArray<Mappable> mappables;

    void addNodesRecursive (const Node& node)
    {
        for (int j = 0; j < node.getNumNodes(); ++j)
        {
            const auto child (node.getNode (j));
            if (GraphNode* const object = child.getGraphNode())
                mappables.add (new Mappable (*this, child));
            if (child.getNumChildren() > 0)
                addNodesRecursive (child);
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
        return object != nullptr && 
            (parameter == GraphNode::EnabledParameter || 
             parameter == GraphNode::BypassParameter || 
             parameter == GraphNode::MuteParameter ||
             isPositiveAndBelow (parameter, object->getParameters().size())) &&
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

    if (shouldLearn)
    {
        DBG("[EL] MappingController: start learning");
        impl->learnState = CaptureParameter;
        capture.addNodes (getWorld().getSession());
    }
}

void MappingController::onParameterCaptured (const Node& node, int parameter)
{
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
                    gui->stabilizeViews();
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
