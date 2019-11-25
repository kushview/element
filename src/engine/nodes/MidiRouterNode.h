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

#pragma once

#include "engine/GraphNode.h"
#include "engine/LinearFade.h"
#include "engine/ToggleGrid.h"
#include "engine/nodes/BaseProcessor.h"

namespace Element {

class MidiRouterNode : public GraphNode,
                       public ChangeBroadcaster
{
public:
    explicit MidiRouterNode (int ins = 4, int outs = 4);
    ~MidiRouterNode();

    void prepareToRender (double sampleRate, int maxBufferSize) override { ignoreUnused (sampleRate, maxBufferSize); }
    void releaseResources() override { }

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
        if (auto* prog = programs [index])
            return prog->name;
        return "MIDI Router " + String (index + 1); 
    }

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier   = EL_INTERNAL_ID_MIDI_ROUTER;
        desc.uid                = EL_INTERNAL_UID_MIDI_ROUTER;
        desc.name               = "MIDI Router";
        desc.descriptiveName    = "A MIDI Patch Grid";
        desc.numInputChannels   = numDestinations;
        desc.numOutputChannels  = numSources;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }

protected:
    inline void createPorts() override
    {
        if (ports.size() > 0)
            return;
        int index = 0;

        ports.add (PortType::Midi, index++, 0, "midi_in_0", "Input 1", true);
        ports.add (PortType::Midi, index++, 1, "midi_in_1", "Input 2", true);
        ports.add (PortType::Midi, index++, 2, "midi_in_2", "Input 3", true);
        ports.add (PortType::Midi, index++, 3, "midi_in_3", "Input 4", true);

        ports.add (PortType::Midi, index++, 0, "midi_out_0", "Output 1", false);
        ports.add (PortType::Midi, index++, 1, "midi_out_1", "Output 2", false);
        ports.add (PortType::Midi, index++, 2, "midi_out_2", "Output 3", false);
        ports.add (PortType::Midi, index++, 3, "midi_out_3", "Output 4", false);
    }

private:
    CriticalSection lock;
    const int numSources;
    const int numDestinations;
    
    struct Program
    {
        Program (const String& programName, int midiProgramNumber = -1)
            : name (programName), midiProgram (midiProgramNumber) { }
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
};

}
