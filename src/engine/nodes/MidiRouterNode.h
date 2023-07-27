// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "engine/nodes/NodeTypes.h"
#include <element/processor.hpp>
#include "engine/linearfade.hpp"
#include "engine/togglegrid.hpp"

namespace element {

class MidiRouterNode : public Processor,
                       public ChangeBroadcaster
{
public:
    explicit MidiRouterNode (int ins = 4, int outs = 4);
    ~MidiRouterNode();

    void prepareToRender (double sampleRate, int maxBufferSize) override { ignoreUnused (sampleRate, maxBufferSize); }
    void releaseResources() override {}

    inline bool wantsMidiPipe() const override { return true; }
    void render (AudioSampleBuffer&, MidiPipe&) override;
    void getState (MemoryBlock&) override;
    void setState (const void*, int sizeInBytes) override;

    void setMatrixState (const MatrixState&);
    MatrixState getMatrixState() const;
    void setWithoutLocking (int src, int dst, bool set);
    CriticalSection& getLock() { return lock; }

    int getNumPrograms() const override { return jmax (1, programs.size()); }
    int getCurrentProgram() const override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) const override
    {
        if (auto* prog = programs[index])
            return prog->name;
        return "MIDI Router " + String (index + 1);
    }

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_NODE_ID_MIDI_ROUTER;
        desc.uniqueId = EL_NODE_UID_MIDI_ROUTER;
        desc.name = "MIDI Router";
        desc.descriptiveName = "A MIDI Patch Grid";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    void refreshPorts() override
    {
        if (getNumPorts() > 0)
            return;
        int index = 0;
        PortList newPorts;
        newPorts.add (PortType::Midi, index++, 0, "midi_in_0", "Input 1", true);
        newPorts.add (PortType::Midi, index++, 1, "midi_in_1", "Input 2", true);
        newPorts.add (PortType::Midi, index++, 2, "midi_in_2", "Input 3", true);
        newPorts.add (PortType::Midi, index++, 3, "midi_in_3", "Input 4", true);

        newPorts.add (PortType::Midi, index++, 0, "midi_out_0", "Output 1", false);
        newPorts.add (PortType::Midi, index++, 1, "midi_out_1", "Output 2", false);
        newPorts.add (PortType::Midi, index++, 2, "midi_out_2", "Output 3", false);
        newPorts.add (PortType::Midi, index++, 3, "midi_out_3", "Output 4", false);
        setPorts (newPorts);
    }

private:
    CriticalSection lock;
    const int numSources;
    const int numDestinations;

    struct Program
    {
        Program (const String& programName, int midiProgramNumber = -1)
            : name (programName), midiProgram (midiProgramNumber) {}
        String name { "1 to 1" };
        int midiProgram { -1 };
        MatrixState matrix;
    };

    OwnedArray<Program> programs;
    int currentProgram = -1;

    void set (int src, int dst, bool patched);
    void clearPatches();

    // used by the UI, but not the rendering
    MatrixState state;

    ToggleGrid toggles;
    ToggleGrid nextToggles;
    bool togglesChanged { false };

    OwnedArray<MidiBuffer> midiOuts;
    void initMidiOuts (OwnedArray<MidiBuffer>& outs);
};

} // namespace element
