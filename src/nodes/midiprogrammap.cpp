// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nodes/midiprogrammap.hpp"
#include "engine/trace.hpp"

namespace element {

MidiProgramMapNode::MidiProgramMapNode()
    : MidiFilterNode (0)
{
    setName ("Midi Program Map");
}

MidiProgramMapNode::~MidiProgramMapNode() {}

void MidiProgramMapNode::clear()
{
    entries.clearQuick (true);
    ScopedLock sl (lock);
    for (int i = 0; i < 128; ++i)
        programMap[i] = -1;
}

void MidiProgramMapNode::prepareToRender (double sampleRate, int maxBufferSize)
{
    ignoreUnused (sampleRate, maxBufferSize);

    {
        ScopedLock sl (lock);
        for (int i = 0; i < 128; ++i)
            programMap[i] = -1;
        for (const auto* const entry : entries)
            programMap[entry->in] = entry->out;
    }
}

void MidiProgramMapNode::releaseResources() {}

void MidiProgramMapNode::render (RenderContext& rc)
{
    if (rc.midi.getNumBuffers() <= 0)
    {
        if (! assertedLowChannels)
        {
            DBG ("[element] PGC map: num bufs: " << rc.midi.getNumBuffers());
            assertedLowChannels = true;
        }

        return;
    }

    auto* const midiIn = rc.midi.getWriteBuffer (0);

    ScopedLock sl (lock);
    MidiMessage msg;

    if (! toSendMidi.isEmpty())
    {
        for (auto m : toSendMidi)
            midiIn->addEvent (m.data, m.numBytes, m.samplePosition);
        toSendMidi.clear();
    }

    int program = -1;

    for (auto m : *midiIn)
    {
        msg = m.getMessage();
        if (msg.isProgramChange() && programMap[msg.getProgramChangeNumber()] >= 0)
        {
            program = msg.getProgramChangeNumber();
            tempMidi.addEvent (MidiMessage::programChange (
                                   msg.getChannel(), programMap[msg.getProgramChangeNumber()]),
                               m.samplePosition);
        }
        else
        {
            tempMidi.addEvent (msg, m.samplePosition);
        }
    }

    if (program >= 0 && program != lastProgram)
    {
        lastProgram = program;
        triggerAsyncUpdate();
    }

    midiIn->swapWith (tempMidi);
    // traceMidi (*midiIn);
    tempMidi.clear();
}

void MidiProgramMapNode::sendProgramChange (int program, int channel)
{
    const auto msg (MidiMessage::programChange (channel, program));
    ScopedLock sl (lock);
    toSendMidi.addEvent (msg, 0);
}

int MidiProgramMapNode::getNumProgramEntries() const { return entries.size(); }

void MidiProgramMapNode::addProgramEntry (const String& name, int programIn, int programOut)
{
    if (programIn < 0)
        programIn = 0;
    if (programIn > 127)
        programIn = 127;
    if (programOut < 0)
        programOut = programIn;
    if (programOut > 127)
        programOut = 127;

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
    entry->in = programIn;
    entry->out = programOut;
    sendChangeMessage();

    ScopedLock sl (lock);
    programMap[entry->in] = entry->out;
}

void MidiProgramMapNode::editProgramEntry (int index, const String& name, int inProgram, int outProgram)
{
    if (auto* entry = entries[index])
    {
        entry->name = name.isNotEmpty() ? name : entry->name;
        entry->in = inProgram;
        entry->out = outProgram;
        ScopedLock sl (lock);
        programMap[entry->in] = entry->out;
        sendChangeMessage();
    }
}

void MidiProgramMapNode::removeProgramEntry (int index)
{
    std::unique_ptr<ProgramEntry> deleter;
    if (auto* const entry = entries[index])
    {
        entries.remove (index, false);
        deleter.reset (entry);
        ScopedLock sl (lock);
        programMap[entry->in] = -1;
        sendChangeMessage();
    }
}

MidiProgramMapNode::ProgramEntry MidiProgramMapNode::getProgramEntry (int index) const
{
    if (auto* const entry = entries[index])
        return *entry;
    return {};
}

} // namespace element
