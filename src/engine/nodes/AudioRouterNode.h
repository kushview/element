#pragma once

#include "engine/GraphNode.h"
#include "engine/LinearFade.h"
#include "engine/ToggleGrid.h"
#include "engine/nodes/BaseProcessor.h"

namespace Element {

class AudioRouterNode : public GraphNode,
                        public ChangeBroadcaster
{
public:
    explicit AudioRouterNode (int ins = 4, int outs = 4);
    ~AudioRouterNode();

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
        desc.fileOrIdentifier   = EL_INTERNAL_ID_AUDIO_ROUTER;
        desc.name               = "Audio Router";
        desc.descriptiveName    = "An Audio Patch Grid";
        desc.numInputChannels   = numDestinations;
        desc.numOutputChannels  = numSources;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
        desc.uid                = EL_INTERNAL_UID_AUDIO_ROUTER;
    }

protected:
    inline void createPorts() override
    {
        if (ports.size() > 0)
            return;
        int index = 0;

        ports.add (PortType::Audio, index++, 0, "audio_in_0", "Input 1", true);
        ports.add (PortType::Audio, index++, 1, "audio_in_1", "Input 2", true);
        ports.add (PortType::Audio, index++, 2, "audio_in_2", "Input 3", true);
        ports.add (PortType::Audio, index++, 3, "audio_in_3", "Input 4", true);

        ports.add (PortType::Audio, index++, 0, "audio_out_0", "Output 1", false);
        ports.add (PortType::Audio, index++, 1, "audio_out_1", "Output 2", false);
        ports.add (PortType::Audio, index++, 2, "audio_out_2", "Output 3", false);
        ports.add (PortType::Audio, index++, 3, "audio_out_3", "Output 4", false);

        ports.add (PortType::Midi, index++, 0, "midi_in",  "MIDI In",  true);
    }

private:
    CriticalSection lock;
    const int numSources;
    const int numDestinations;
    AudioSampleBuffer tempAudio { 1, 1 };
    
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

    double fadeLengthSeconds { 0.001 }; // 1 ms
    LinearFade fadeIn;
    LinearFade fadeOut;
    ToggleGrid toggles;
    ToggleGrid nextToggles;
    bool togglesChanged { false };
};

}
