// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/graph.hpp>

using namespace juce;

namespace element {

Graph Graph::create (const juce::String& name, int numAudioIns, int numAudioOuts, bool midiIn, bool midiOut)
{
    Node graph (types::Graph);
    graph.setProperty (tags::name, name);
    ValueTree gports = graph.getPortsValueTree();

    int portIdx = 0;
    for (int c = 0; c < numAudioIns; c++)
    {
        Port port (String ("Audio In XX").replace ("XX", String (c + 1)),
                   tags::audio,
                   tags::input,
                   portIdx++);
        gports.addChild (port.data(), -1, nullptr);
    }

    if (midiIn)
        gports.addChild (Port ("MIDI In", tags::midi, tags::input, portIdx++).data(), -1, 0);

    for (int c = 0; c < numAudioOuts; c++)
    {
        Port port (String ("Audio Out XX").replace ("XX", String (c + 1)),
                   tags::audio,
                   tags::output,
                   portIdx++);
        gports.addChild (port.data(), -1, nullptr);
    }

    if (midiOut)
        gports.addChild (Port ("MIDI Out", tags::midi, tags::output, portIdx++).data(), -1, 0);

    return graph;
}

} // namespace element