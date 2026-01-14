// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/element.h>
#include <element/node.hpp>
#include <element/script.hpp>

namespace element {

class EL_API Graph : public Node {
public:
    /** Make an invalid graph. */
    Graph() : Node() {}
    Graph (const Node& node) : Node (node.data(), false)
    {
        if (isValid()) {
            jassert (getProperty (tags::type) == types::Graph.toString());
        }
    }

    Graph (const Node& node, bool init) : Node (node.data(), init)
    {
        if (isValid()) {
            jassert (getProperty (tags::type) == types::Graph.toString());
        }
    }

    inline Script findViewScript() const noexcept
    {
        auto scripts = getScriptsValueTree();
        return scripts.getChildWithProperty (tags::type, types::View.toString());
    }

    inline bool hasViewScript() const noexcept { return findViewScript().valid(); }

    /** Create a graph with X amount of audio ins, outs, and/or MIDI ports.
        
        @param name The name of the graph.
        @param numAudioIns Total number of audio in ports.
        @param numAudioOuts Total number of audio out ports.
        @param midiIn  If true, add a MIDI input.
        @param midiOut If true, add a MIDI output.
    */
    static Graph create (const juce::String& name, int numAudioIns, int numAudioOuts, bool midiIn, bool midiOut);
};

} // namespace element
