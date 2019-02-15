#pragma once

#include "engine/nodes/MidiFilterNode.h"
#include "engine/MidiPipe.h"
#include "engine/BaseProcessor.h"

namespace Element {

class ProgramChangeMapNode : public MidiFilterNode
{
public:
    struct ProgramEntry
    {
        String name;
        int in;
        int out;
    };
    
    ProgramChangeMapNode();
    virtual ~ProgramChangeMapNode();

    void clear();
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void sendProgramChange (int program, int channel);
    int getNumProgramEntries() const;
    void addProgramEntry (const String& name, int programIn, int programOut = -1);
    void removeProgramEntry (int index);
    void editProgramEntry (int index, const String& name, int inProgram, int outProgram);
    ProgramEntry getProgramEntry (int index) const;

protected:
    CriticalSection lock;
    OwnedArray<ProgramEntry> entries;
    int programMap [127];

    bool assertedLowChannels = false;
    bool createdPorts = false;
    MidiBuffer* buffers [16];
    MidiBuffer tempMidi;
    MidiBuffer toSendMidi;

    int width = 360;
    int height = 640;

    inline void createPorts() override
    {
        if (createdPorts)
            return;

        ports.clearQuick();
        ports.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        ports.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
        createdPorts = true;
    }
};

}
