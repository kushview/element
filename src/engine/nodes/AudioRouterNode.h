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

#include <element/nodeobject.hpp>
#include "engine/linearfade.hpp"
#include "engine/togglegrid.hpp"
#include "engine/nodes/BaseProcessor.h"

namespace element {

class AudioRouterNode : public NodeObject,
                        public ChangeBroadcaster
{
public:
    explicit AudioRouterNode (int ins = 4, int outs = 4);
    ~AudioRouterNode();

    void prepareToRender (double sampleRate, int maxBufferSize) override { ignoreUnused (sampleRate, maxBufferSize); }
    void releaseResources() override {}

    inline bool wantsMidiPipe() const override { return true; }
    void render (AudioSampleBuffer&, MidiPipe&) override;
    void getState (MemoryBlock&) override;
    void setState (const void*, int sizeInBytes) override;

    void setSize (int newIns, int newOuts);
    String getSizeString() const;
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
        return "Audio Router " + String (index + 1);
    }

    void setFadeLength (double seconds)
    {
        seconds = jlimit (0.001, 5.0, seconds);
        ScopedLock sl (lock);
        fadeLengthSeconds = seconds;
        fadeIn.setLength (static_cast<float> (fadeLengthSeconds));
        fadeOut.setLength (static_cast<float> (fadeLengthSeconds));
    }

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_INTERNAL_ID_AUDIO_ROUTER;
        desc.name = "Audio Router";
        desc.descriptiveName = "An Audio Patch Grid";
        desc.numInputChannels = numDestinations;
        desc.numOutputChannels = numSources;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = "Element";
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
        desc.uniqueId = EL_INTERNAL_UID_AUDIO_ROUTER;
    }

    inline void refreshPorts() override
    {
        if (getNumPorts() > 0 && ! rebuildPorts)
            return;

        PortList newPorts;
        int index = 0;
        int channel = 0;
        for (int i = 0; i < numSources; ++i)
        {
            newPorts.add (PortType::Audio, index++, channel++, String ("audio_in_XX").replace ("XX", String (i)), String ("Input XX").replace ("XX", String (i + 1)), true);
        }

        channel = 0;
        for (int i = 0; i < numSources; ++i)
        {
            newPorts.add (PortType::Audio, index++, channel++, String ("audio_out_XX").replace ("XX", String (i)), String ("Output XX").replace ("XX", String (i + 1)), false);
        }

        newPorts.add (PortType::Midi, index++, 0, "midi_in", "MIDI In", true);
        rebuildPorts = false;
        setPorts (newPorts);
    }

private:
    CriticalSection lock;
    int numSources, nextNumSources;
    int numDestinations, nextNumDestinations;
    AudioSampleBuffer tempAudio { 1, 1 };
    bool rebuildPorts = true;

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

    double fadeLengthSeconds { 0.001 }; // 1 ms
    LinearFade fadeIn;
    LinearFade fadeOut;
    ToggleGrid toggles;
    ToggleGrid nextToggles;
    bool togglesChanged { false },
        sizeChanged { false };

    void applyMatrix (const MatrixState&);
};

} // namespace element
