#pragma once

#include "engine/GraphNode.h"

// https://stackoverflow.com/questions/936687/how-do-i-declare-a-2d-array-in-c-using-new

namespace Element {

class AudioRouterNode : public GraphNode
{
public:
    explicit AudioRouterNode (int ins = 4, int outs = 4);
    ~AudioRouterNode();

    inline bool wantsMidiPipe() const override { return true; }
    void render (AudioSampleBuffer&, MidiPipe&) override;
    void getState (MemoryBlock&) override;
    void setState (const void*, int sizeInBytes) override;

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
    const int numSources;
    const int numDestinations;
    bool** patches { nullptr };
    AudioSampleBuffer tempAudio { 1, 1 };
    struct Program
    {
        int program { 0 };
        String name { "" };
        MatrixState matrix;
    };

    OwnedArray<Program> programs;

    void set (int src, int dst, bool patched);
    void clearPatches();
};

}
