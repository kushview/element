
#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/AudioRouterNode.h"
#include "Common.h"

namespace Element {

AudioRouterNode::AudioRouterNode (int ins, int outs)
    : GraphNode (0),
      numSources (ins),
      numDestinations (outs),
      toggles (ins, outs),
      nextToggles (ins, outs)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_AUDIO_ROUTER, nullptr);
    
    patches = new bool* [numSources];
    for (int i = 0; i < numSources; ++i)
        patches[i] = new bool [numDestinations];

    fadeIn.setFadesIn (true);
    fadeIn.setLength (fadeLengthSeconds);
    fadeOut.setFadesIn (false);
    fadeOut.setLength (fadeLengthSeconds);

    clearPatches();
    for (int i = 0; i < jmin (ins, outs); ++i)
        setWithoutLocking (i, i, true);
}

AudioRouterNode::~AudioRouterNode()
{
    for (int i = 0; i < numSources; ++i)
        delete [] patches[i];
    delete [] patches;
}

MatrixState AudioRouterNode::getMatrixState() const
{
    MatrixState state (numSources, numDestinations);
    
    {
        ScopedLock sl (lock);
        for (int r = 0; r < numSources; ++r)
            for (int c = 0; c < numDestinations; ++c)
                state.set (r, c, patches[r][c]);
    }

    return state;
}

void AudioRouterNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    jassert (midi.getNumBuffers() == 1);
    const auto& midiBuffer = *midi.getReadBuffer (0);
    MidiBuffer::Iterator iter (midiBuffer);
    ignoreUnused (iter);
    tempAudio.setSize (audio.getNumChannels(), audio.getNumSamples(),
                       false, false, true);
    tempAudio.clear (0, audio.getNumSamples());

    {
        ScopedLock sl (lock);
        for (int i = 0; i < numSources; ++i)
        {
            for (int j = 0; j < numDestinations; ++j)
            {
                if (patches[i][j])
                {
                    tempAudio.addFrom (j, 0, audio, i, 0, audio.getNumSamples());
                }
            }
        }
    }

    for (int c = 0; c < audio.getNumChannels(); ++c)
        audio.copyFrom (c, 0, tempAudio.getReadPointer(c), audio.getNumSamples());
    midi.clear();
}

void AudioRouterNode::getState (MemoryBlock& block)
{
    MemoryOutputStream stream (block, false);
    const auto state = getMatrixState();
    state.createValueTree().writeToStream (stream);
}

void AudioRouterNode::setState (const void* data, int sizeInBytes)
{ 
    const auto tree = ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (tree.isValid())
    {
        kv::MatrixState matrix;
        matrix.restoreFromValueTree (tree);
        jassert (matrix.getNumRows() == numSources && matrix.getNumColumns() == numDestinations);
        
        {
            ScopedLock sl (lock);
            for (int r = 0; r < numSources; ++r)
                for (int c = 0; c < numDestinations; ++c)
                    patches[r][c] = matrix.connected (r, c);
        }

        sendChangeMessage();
    }
}

void AudioRouterNode::setWithoutLocking (int src, int dst, bool set)
{
    jassert (src >= 0 && src < 4 && dst >= 0 && dst < 4);
    patches[src][dst] = set;
}

void AudioRouterNode::set (int src, int dst, bool patched)
{
    jassert (src >= 0 && src < 4 && dst >= 0 && dst < 4);
    patches[src][dst] = patched;
}

void AudioRouterNode::clearPatches()
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            patches[i][j] = false;
}

}
