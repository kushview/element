// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nodes/baseprocessor.hpp"
#include "nodes/midirouter.hpp"
#include <element/midipipe.hpp>
#include "common.hpp"

#define TRACE_MIDI_ROUTER(output)

namespace element {

MidiRouterNode::MidiRouterNode (int ins, int outs)
    : Processor (0),
      numSources (ins),
      numDestinations (outs),
      state (ins, outs),
      toggles (ins, outs),
      nextToggles (ins, outs)
{
    setName ("MIDI Router");
    clearPatches();
    initMidiOuts (midiOuts);

    auto* program = programs.add (new Program ("Linear"));
    program->matrix.resize (ins, outs);
    for (int i = 0; i < jmin (ins, outs); ++i)
        program->matrix.set (i, i, true);
    setMatrixState (program->matrix);

    if (ins == 4 && outs == 4)
    {
        program = programs.add (new Program ("1-2 to 1-2"));
        program->matrix.resize (ins, outs);
        program->matrix.set (0, 0, true);
        program->matrix.set (1, 1, true);

        program = programs.add (new Program ("1-2 to 3-4"));
        program->matrix.resize (ins, outs);
        program->matrix.set (0, 2, true);
        program->matrix.set (1, 3, true);

        program = programs.add (new Program ("3-4 to 1-2"));
        program->matrix.resize (ins, outs);
        program->matrix.set (2, 0, true);
        program->matrix.set (3, 1, true);

        program = programs.add (new Program ("3-4 to 3-4"));
        program->matrix.resize (ins, outs);
        program->matrix.set (2, 2, true);
        program->matrix.set (3, 3, true);
    }
}

MidiRouterNode::~MidiRouterNode() {}

void MidiRouterNode::setCurrentProgram (int index)
{
    if (auto* program = programs[index])
    {
        currentProgram = index;
        setMatrixState (program->matrix);
    }
}

void MidiRouterNode::setMatrixState (const MatrixState& matrix)
{
    jassert (state.sameSizeAs (matrix));
    state = matrix;
    ToggleGrid newPatches (state);

    {
        ScopedLock sl (getLock());
        toggles.swapWith (newPatches);
        togglesChanged = true; // initiate the crossfade
    }

    sendChangeMessage();
}

MatrixState MidiRouterNode::getMatrixState() const
{
    return state;
}

void MidiRouterNode::render (RenderContext& rc)
{
    jassert (rc.midi.getNumBuffers() >= numDestinations);

    const auto nsamples = rc.audio.getNumSamples();
    const auto nbuffers = rc.midi.getNumBuffers();
    rc.audio.clear();

    ScopedLock sl (getLock());
    for (int src = 0; src < numSources; ++src)
    {
        if (src >= nbuffers)
            break;

        const auto& rb = *rc.midi.getReadBuffer (src);
        for (int dst = 0; dst < numDestinations; ++dst)
            if (toggles.get (src, dst))
                midiOuts.getUnchecked (dst)->addEvents (rb, 0, nsamples, 0);
    }

    for (int i = midiOuts.size(); --i >= 0;)
    {
        auto* const ob = midiOuts.getUnchecked (i);
        ob->swapWith (*rc.midi.getWriteBuffer (i));
        ob->clear();
    }
}

void MidiRouterNode::getState (MemoryBlock& block)
{
    MemoryOutputStream stream (block, false);
    state.createValueTree().writeToStream (stream);
}

void MidiRouterNode::setState (const void* data, int sizeInBytes)
{
    const auto tree = ValueTree::readFromData (data, (size_t) sizeInBytes);

    if (tree.isValid())
    {
        MatrixState matrix;
        matrix.restoreFromValueTree (tree);
        jassert (matrix.getNumRows() == numSources && matrix.getNumColumns() == numDestinations);
        setMatrixState (matrix);
    }
}

void MidiRouterNode::setWithoutLocking (int src, int dst, bool set)
{
    jassert (src >= 0 && src < numSources && dst >= 0 && dst < numDestinations);
    toggles.set (src, dst, set);
    state.set (src, dst, set);
}

void MidiRouterNode::set (int src, int dst, bool patched)
{
    jassert (src >= 0 && src < numSources && dst >= 0 && numDestinations < 4);
    toggles.set (src, dst, patched);
    state.set (src, dst, patched);
}

void MidiRouterNode::clearPatches()
{
    {
        ScopedLock sl (getLock());
        toggles.clear();
        nextToggles.clear();
    }

    for (int r = 0; r < state.getNumRows(); ++r)
        for (int c = 0; c < state.getNumColumns(); ++c)
            state.set (r, c, false);
}

void MidiRouterNode::initMidiOuts (OwnedArray<MidiBuffer>& outs)
{
    while (outs.size() < numDestinations)
    {
        auto* const buf = outs.add (new MidiBuffer());
        buf->ensureSize (16 * 3);
    }
}

} // namespace element
