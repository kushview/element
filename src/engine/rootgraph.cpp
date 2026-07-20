// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "engine/rootgraph.hpp"

namespace element {

RootGraph::RootGraph (Context& c)
    : GraphNode (c) {}

void RootGraph::refreshPorts()
{
    GraphNode::refreshPorts();
}

void RootGraph::setEngineManaged (bool managed)
{
    if (engineManaged == managed)
        return;

    engineManaged = managed;

    if (prepared())
        rebuild();
}

void RootGraph::handleAsyncUpdate()
{
    if (engineManaged)
    {
        // The engine schedules this graph inside its unified merged schedule, so
        // build no ops of our own: control-port BindParameterOps must exist
        // exactly once and the merged schedule owns them. Still notify so the
        // engine rebuilds the merged schedule.
        clearRenderingSequence();
        renderingSequenceChanged();
        return;
    }

    GraphNode::handleAsyncUpdate();
}

void RootGraph::setPlayConfigFor (DeviceManager& devices)
{
    if (auto* const device = devices.getCurrentAudioDevice())
        setPlayConfigFor (device);
}

void RootGraph::setPlayConfigFor (AudioIODevice* device)
{
    jassert (device != nullptr);
    setRenderDetails (device->getCurrentBufferSizeSamples(),
                      device->getCurrentSampleRate());
}

void RootGraph::setPlayConfigFor (const DeviceManager::AudioDeviceSetup& setup)
{
    setRenderDetails (setup.sampleRate, setup.bufferSize);
}

} // namespace element
