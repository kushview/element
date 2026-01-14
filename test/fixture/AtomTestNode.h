// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/processor.hpp>
#include <element/atombuffer.hpp>

namespace element {

/** Simple test node with atom port support */
class AtomTestNode : public Processor {
public:
    AtomTestNode (int midiIns = 0, int midiOuts = 0, int atomIns = 0, int atomOuts = 0)
        : Processor (0),
          numMidiIns (midiIns),
          numMidiOuts (midiOuts),
          numAtomIns (atomIns),
          numAtomOuts (atomOuts)
    {
        AtomTestNode::refreshPorts();
    }

    ~AtomTestNode() = default;

    void prepareToRender (double newSampleRate, int newBlockSize) override
    {
        setRenderDetails (newSampleRate, newBlockSize);
    }

    void releaseResources() override {}

    void render (RenderContext& rc) override
    {
        // Pass through by default - subclasses can override
    }

    void getState (MemoryBlock&) override {}
    void setState (const void*, int) override {}

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.pluginFormatName = "Element";
        desc.fileOrIdentifier = "element.atomTestNode";
        desc.manufacturerName = "Element";
    }

    void refreshPorts() override
    {
        PortList newPorts;
        uint32 port = 0;

        for (int i = 0; i < numMidiIns; i++)
            newPorts.add (PortType::Midi, port++, i, "midi_in_" + String (i), "MIDI In " + String (i + 1), true);

        for (int i = 0; i < numAtomIns; i++)
            newPorts.add (PortType::Atom, port++, i, "atom_in_" + String (i), "Atom In " + String (i + 1), true);

        for (int i = 0; i < numMidiOuts; i++)
            newPorts.add (PortType::Midi, port++, i, "midi_out_" + String (i), "MIDI Out " + String (i + 1), false);

        for (int i = 0; i < numAtomOuts; i++)
            newPorts.add (PortType::Atom, port++, i, "atom_out_" + String (i), "Atom Out " + String (i + 1), false);

        setPorts (newPorts);
    }

protected:
    int numMidiIns, numMidiOuts, numAtomIns, numAtomOuts;

    void initialize() override {}
};

} // namespace element
