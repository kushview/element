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
#include "engine/nodes/AudioRouterNode.h"
#include "Common.h"

#define TRACE_AUDIO_ROUTER(output) 

namespace Element {

AudioRouterNode::AudioRouterNode (int ins, int outs)
    : GraphNode (0),
      numSources (ins),
      numDestinations (outs),
      state (ins, outs),
      toggles (ins, outs),
      nextToggles (ins, outs)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_AUDIO_ROUTER, nullptr);
    
    fadeIn.setFadesIn (true);
    fadeIn.setLength (fadeLengthSeconds);
    fadeOut.setFadesIn (false);
    fadeOut.setLength (fadeLengthSeconds);

    clearPatches();

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

AudioRouterNode::~AudioRouterNode() { }

void AudioRouterNode::setCurrentProgram (int index)
{
    if (auto* program = programs [index])
    {
        currentProgram = index;
        setMatrixState (program->matrix);
    }
}

void AudioRouterNode::applyMatrix (const MatrixState& matrix)
{
    jassert (matrix.sameSizeAs (state));
    ToggleGrid newPatches (matrix);
    {
        ScopedLock sl (getLock());
        nextToggles.swapWith (newPatches);
        togglesChanged = true; // initiate the crossfade
    }

    sendChangeMessage();
}

String AudioRouterNode::getSizeString() const
{
    int s = 0, d = 0;
    {
        ScopedLock sl (lock);
        s = numSources;
        d = numDestinations;
    }

    String result (s);
    result << "x" << d;
    return result;
}

void AudioRouterNode::setSize (int newIns, int newOuts)
{
    newIns  = jmax (1, newIns);
    newOuts = jmax (1, newOuts);
    
    {
        ScopedLock sl1 (getLock());
        if (newIns == numSources && newOuts == numDestinations)
            return;
    }

    state.resize (newIns, newOuts);
    ToggleGrid newPatches (state);
    ToggleGrid newNextPatches (state);
    
    {
        ScopedLock sl (getLock());
        nextToggles.swapWith (newNextPatches);
        toggles.swapWith (newPatches);
        numSources = newIns;
        numDestinations = newOuts;
        sizeChanged = true; // initiate the size change
    }

    rebuildPorts = true;
    triggerPortReset();
    sendChangeMessage();
}

void AudioRouterNode::setMatrixState (const MatrixState& matrix)
{
    state = matrix;
    applyMatrix (state);
}

MatrixState AudioRouterNode::getMatrixState() const
{
    return state;
}

void AudioRouterNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    jassert (midi.getNumBuffers() == 1);
    const auto& midiBuffer = *midi.getReadBuffer (0);

    MidiBuffer::Iterator iter (midiBuffer);
    MidiMessage msg; int midiFrame = 0;
    
    while (iter.getNextEvent (msg, midiFrame))
    {
        if (! msg.isProgramChange())
            continue;
        if (3 == msg.getProgramChangeNumber())
            { DBG("program "); }
    }

    const int numFrames = audio.getNumSamples();
    const int numChannels = audio.getNumChannels();

    tempAudio.setSize (numChannels, numFrames, false, false, true);
    tempAudio.clear (0, numFrames);

    if (sizeChanged)
    {
        fadeIn.reset();
        fadeOut.reset();
        sizeChanged = false;
        TRACE_AUDIO_ROUTER("size changed");
    }

    if (togglesChanged)
    {
        fadeIn.reset();
        fadeIn.startFading();
        fadeOut.reset();
        fadeOut.startFading();
        togglesChanged = false;
        TRACE_AUDIO_ROUTER("fade start");
    }

    if (numSources > numChannels || numDestinations > numChannels)
    {
        audio.clear();
        midi.clear();
        return;
    }

    if (fadeIn.isActive() || fadeOut.isActive())
    {
        auto framesToProcess = numFrames;
        int frame = 0;
        ScopedLock sl (lock);
        
        float fadeInGain  = 0.0f;
        float fadeOutGain = 1.0f;

        while (--framesToProcess >= 0)
        {
            fadeInGain  = fadeIn.isActive()  ? fadeIn.getNextEnvelopeValue()  : 1.0f;
            fadeOutGain = fadeOut.isActive() ? fadeOut.getNextEnvelopeValue() : 0.0f;

            if (!fadeIn.isActive() && !fadeOut.isActive())
            {
                TRACE_AUDIO_ROUTER("last frame fade in gain  : " << fadeInGain);
                TRACE_AUDIO_ROUTER("last frame fade out gain : " << fadeOutGain);
            }

            for (int i = 0; i < numSources; ++i)
            {
                for (int j = 0; j < numDestinations; ++j)
                {
                    if (toggles.get (i, j) && nextToggles.get (i, j))
                    {
                        // no patch change and on means 1 to 1 mix
                        tempAudio.getWritePointer(j)[frame] += 
                            audio.getReadPointer(i)[frame];
                    }
                    else if (! toggles.get (i, j) && ! nextToggles.get (i, j))
                    {
                        // no patch change and off means no fade and zero'd
                        tempAudio.getWritePointer(j)[frame] += 0.0f;
                    }
                    else if (!toggles.get (i, j) && nextToggles.get (i, j))
                    {
                        tempAudio.getWritePointer(j)[frame] += 
                            (audio.getReadPointer(i)[frame] * fadeInGain);
                    }
                    else if (toggles.get (i, j) && !nextToggles.get (i, j))
                    {
                        tempAudio.getWritePointer(j)[frame] += 
                            (audio.getReadPointer(i)[frame] * fadeOutGain);
                    }
                }
            }

            ++frame;

            if (!fadeIn.isActive() && !fadeOut.isActive())
                break;
        }

        if (!fadeOut.isActive() && !fadeIn.isActive())
        {
            TRACE_AUDIO_ROUTER("fade stopped @ frame: " << (frame));
            TRACE_AUDIO_ROUTER("fade in level  : " << fadeInGain);
            TRACE_AUDIO_ROUTER("fade out level : " << fadeOutGain);

            if (framesToProcess > 0)
            {
                TRACE_AUDIO_ROUTER("rendering " << framesToProcess << " remainging frames");
                for (int i = 0; i < numSources; ++i)
                {
                    for (int j = 0; j < numDestinations; ++j)
                    {
                        if (toggles.get(i, j) && nextToggles.get (i, j))
                        {
                            // no patch change and on means 1 to 1 mix
                            tempAudio.addFrom (j, frame, audio.getReadPointer (i, frame), 
                                               framesToProcess);
                        }
                        else if (! toggles.get (i, j) && ! nextToggles.get (i, j))
                        {
                            // patch not changed and of, so clear the out channel.
                            // nothing to do
                        }
                        else if (!toggles.get (i, j) && nextToggles.get (i, j))
                        {
                            tempAudio.addFromWithRamp (j, frame, audio.getReadPointer (i, frame), 
                                                          framesToProcess, fadeInGain, 1.0f);
                        }
                        else if (toggles.get (i, j) && !nextToggles.get (i, j))
                        {
                            tempAudio.addFromWithRamp (j, frame, audio.getReadPointer (i, frame),
                                                          framesToProcess, fadeOutGain, 0.0f);
                        }
                        else
                        {
                            // this would be an unhandled fading scenario
                            jassertfalse;
                        }
                    }
                }
            }

            toggles.swapWith (nextToggles);
        }
    }
    else
    {
        ScopedLock sl (lock);
        for (int i = 0; i < numSources; ++i)
            for (int j = 0; j < numDestinations; ++j)
                if (toggles.get (i, j))
                    tempAudio.addFrom (j, 0, audio, i, 0, numFrames);
    }

    for (int c = 0; c < numChannels; ++c)
        audio.copyFrom (c, 0, tempAudio.getReadPointer(c), numFrames);
    midi.clear();
}

void AudioRouterNode::getState (MemoryBlock& block)
{
    MemoryOutputStream stream (block, false);
    state.createValueTree().writeToStream (stream);
}

void AudioRouterNode::setState (const void* data, int sizeInBytes)
{ 
    const auto tree = ValueTree::readFromData (data, (size_t) sizeInBytes);
    
    if (tree.isValid())
    {
        kv::MatrixState matrix;
        matrix.restoreFromValueTree (tree);
        jassert (matrix.getNumRows() > 0 && matrix.getNumColumns() > 0);
        if (matrix.getNumRows() > 0 && matrix.getNumColumns() > 0)
        {
            state = matrix;

            ToggleGrid newPatches (state);
            ToggleGrid newNextPatches (state);
            {
                ScopedLock sl (getLock());
                numSources = matrix.getNumRows();
                numDestinations = matrix.getNumColumns();
                nextToggles.swapWith (newNextPatches);
                toggles.swapWith (newPatches);
                sizeChanged = true;
            }

            rebuildPorts = true;
            sendChangeMessage();
            triggerPortReset();
        }
    }
}

void AudioRouterNode::setWithoutLocking (int src, int dst, bool set)
{
    jassert (src >= 0 && src < numSources && dst >= 0 && dst < numDestinations);
    toggles.set (src, dst, set);
    state.set (src, dst, set);
}

void AudioRouterNode::set (int src, int dst, bool patched)
{
    jassert (src >= 0 && src < numSources && dst >= 0 && numDestinations < 4);
    toggles.set (src, dst, patched);
    state.set (src, dst, patched);
}

void AudioRouterNode::clearPatches()
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

}
