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
    
    ProgramChangeMapNode() : MidiFilterNode (0)
    {
        jassert (metadata.hasType (Tags::node));
        metadata.setProperty (Tags::format, "Element", nullptr);
        metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_PROGRAM_CHANGE_MAP, nullptr);
        clear();
    }

    ~ProgramChangeMapNode() { }

    inline void clear()
    {
        entries.clearQuick (true);
        ScopedLock sl (lock);
        for (int i = 0; i <= 127; ++i)
            programMap [i] = -1;
    }

    inline void render (AudioSampleBuffer& audio, MidiPipe& midi) override
    {
        ignoreUnused (audio, midi);
    }

    inline int getNumProgramEntries() const { return entries.size(); }

    inline void addProgramEntry (const String& name, int programIn, int programOut = -1)
    {
        if (programIn < 0)      programIn = 0;
        if (programIn > 127)    programIn = 127;
        if (programOut < 0)     programOut = programIn;
        if (programOut > 127)   programOut = 127;
        
        ProgramEntry* entry = nullptr;
        for (auto* e : entries)
        {
            if (e->in == programIn)
            {
                entry = e;
                break;
            }
        }

        if (entry == nullptr)
            entry = entries.add (new ProgramEntry());

        jassert (entry != nullptr);

        entry->name = name;
        entry->in   = programIn;
        entry->out  = programOut;

        ScopedLock sl (lock);
        programMap [entry->in] = entry->out;
    }

    inline void removeProgramEntry (int index)
    {
        std::unique_ptr<ProgramEntry> deleter;
        if (auto* const entry = entries [index])
        {
            entries.remove (index, false);
            deleter.reset (entry);
            ScopedLock sl (lock);
            programMap[entry->in] = -1;
        }
    }

    inline ProgramEntry getProgramEntry (int index) const
    {
        if (auto* const entry = entries [index])
            return *entry;
        return { };
    }

protected:
    CriticalSection lock;
    OwnedArray<ProgramEntry> entries;
    int programMap [127];

    bool assertedLowChannels = false;
    bool createdPorts = false;
    MidiBuffer* buffers [16];
    MidiBuffer tempMidi;

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
