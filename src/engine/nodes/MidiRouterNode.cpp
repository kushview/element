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

#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/MidiRouterNode.h"
#include "Common.h"

#define TRACE_MIDI_ROUTER(output) 

namespace Element {

MidiRouterNode::MidiRouterNode (int ins, int outs)
    : GraphNode (0),
      numSources (ins),
      numDestinations (outs),
      state (ins, outs),
      toggles (ins, outs),
      nextToggles (ins, outs)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_MIDI_ROUTER, nullptr);

    clearPatches();
    initMidiOuts (midiOuts);

    auto* program = programs.add (new Program ("Linear Stereo"));
    program->matrix.resize (ins, outs);
    for (int i = 0; i < jmin (ins, outs); ++i)
        program->matrix.set (i, i, true);
    setMatrixState (program->matrix);

    if (ins == 4 && outs == 4)
    {
        program = programs.add (new Program ("Inverse Stereo"));
        program->matrix.resize (ins, outs);
        program->matrix.set (0, 1, true);
        program->matrix.set (1, 0, true);
        program->matrix.set (2, 3, true);
        program->matrix.set (3, 2, true);

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

MidiRouterNode::~MidiRouterNode() { }

void MidiRouterNode::setCurrentProgram (int index)
{
    if (auto* program = programs [index])
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

void MidiRouterNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    jassert (midi.getNumBuffers() >= numDestinations);
    
    const auto nsamples = audio.getNumSamples();
    const auto nbuffers = midi.getNumBuffers();
    audio.clear();

    ScopedLock sl (getLock());
    for (int src = 0; src < numSources; ++src)
    {
        if (src >= nbuffers)
            break;
        
        const auto& rb = *midi.getReadBuffer (src);
        for (int dst = 0; dst < numDestinations; ++dst)
            if (toggles.get (src, dst))
                midiOuts.getUnchecked(dst)->addEvents (rb, 0, nsamples, 0);
    }

    for (int i = midiOuts.size(); --i >= 0;)
    {
        auto* const ob = midiOuts.getUnchecked (i);
        ob->swapWith (*midi.getWriteBuffer (i));
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
        kv::MatrixState matrix;
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

}
