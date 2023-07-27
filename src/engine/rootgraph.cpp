// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "engine/rootgraph.hpp"

namespace element {

RootGraph::RootGraph() {}

void RootGraph::refreshPorts()
{
    GraphNode::refreshPorts();
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
