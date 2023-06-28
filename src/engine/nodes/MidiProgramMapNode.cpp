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

#include "ElementApp.h"
#include "engine/nodes/MidiProgramMapNode.h"
#include "engine/trace.hpp"

namespace element {

MidiProgramMapNode::MidiProgramMapNode()
    : MidiFilterNode (0) {}

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

void MidiProgramMapNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    ignoreUnused (audio, midi);
    if (midi.getNumBuffers() <= 0)
    {
        if (! assertedLowChannels)
        {
            DBG ("[element] PGC map: num bufs: " << midi.getNumBuffers());
            assertedLowChannels = true;
        }

        return;
    }

    auto* const midiIn = midi.getWriteBuffer (0);

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
