// Copyright 2024 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ElementApp.h"
#include "nodes/midisetlist.hpp"
#include "engine/trace.hpp"

#include <element/context.hpp>

namespace element {

MidiSetListProcessor::MidiSetListProcessor (Context& ctx)
    : MidiFilterNode (0),
      _context (ctx)
{
    setName ("MIDI Set List");
}

MidiSetListProcessor::~MidiSetListProcessor() {}

void MidiSetListProcessor::clear()
{
    entries.clearQuick (true);
    ScopedLock sl (lock);
    for (int i = 0; i < 128; ++i)
        programMap[i] = -1;
}

void MidiSetListProcessor::prepareToRender (double sampleRate, int maxBufferSize)
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

void MidiSetListProcessor::releaseResources() {}

void MidiSetListProcessor::render (RenderContext& rc)
{
    if (rc.midi.getNumBuffers() <= 0)
    {
        if (! assertedLowChannels)
        {
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
    tempMidi.clear();
}

void MidiSetListProcessor::sendProgramChange (int program, int channel)
{
    const auto msg (MidiMessage::programChange (channel, program));
    ScopedLock sl (lock);
    toSendMidi.addEvent (msg, 0);
}

void MidiSetListProcessor::maybeSendTempoAndPosition (int program)
{
    auto s = _context.session();
    if (s == nullptr)
        return;

    if (auto* entry = entries[program])
    {
        if (entry->tempo >= 20.0)
            s->getValueTree().setProperty (tags::tempo, entry->tempo, nullptr);
        if (auto e = _context.audio())
            e->seekToAudioFrame (0);
    }
}

int MidiSetListProcessor::getNumProgramEntries() const { return entries.size(); }

void MidiSetListProcessor::addProgramEntry (const String& name, int programIn, int programOut)
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
    entry->tempo = 0.0;
    sendChangeMessage();

    ScopedLock sl (lock);
    programMap[entry->in] = entry->out;
}

void MidiSetListProcessor::editProgramEntry (int index,
                                             const String& name,
                                             int inProgram,
                                             int outProgram,
                                             double tempo)
{
    tempo = (tempo <= 0) ? 0.0 : std::max (20.0, std::min (999.0, tempo));
    if (auto* entry = entries[index])
    {
        entry->name = name.isNotEmpty() ? name : entry->name;
        entry->in = inProgram;
        entry->out = outProgram;
        entry->tempo = tempo;
        ScopedLock sl (lock);
        programMap[entry->in] = entry->out;
        sendChangeMessage();
    }
}

void MidiSetListProcessor::removeProgramEntry (int index)
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

MidiSetListProcessor::ProgramEntry MidiSetListProcessor::getProgramEntry (int index) const
{
    if (auto* const entry = entries[index])
        return *entry;
    return {};
}

void MidiSetListProcessor::handleAsyncUpdate()
{
    const auto program = getLastProgram();
    if (isPositiveAndBelow (program, getNumProgramEntries()))
        maybeSendTempoAndPosition (program);
    lastProgramChanged();
}

} // namespace element
