/*
    GraphPlayer.h - This file is part of Element
    Copyright (C) 2014  Kushview, LLC.  All rights reserved.

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

#ifndef ELEMENT_GRAPH_PLAYER_H
#define ELEMENT_GRAPH_PLAYER_H

class GraphProcessor;

class  GraphPlayer : public AudioIODeviceCallback,
                     public MidiInputCallback
{
public:
    GraphPlayer();
    virtual ~GraphPlayer();

    /** Sets the processor that should be played.

        The processor that is passed in will not be deleted or owned by this object.
        To stop anything playing, pass in 0 to this method.
    */
    void setRootGraph (GraphProcessor* graph);

    /** Returns the current audio processor that is being played.
    */
    GraphProcessor* rootGraph() const { return processor; }

    /** Returns a midi message collector that you can pass midi messages to if you
        want them to be injected into the midi stream that is being sent to the
        processor.
    */
    MidiMessageCollector& midiCollector() { return messageCollector; }

    /** @internal */
    void audioDeviceIOCallback (const float** inputChannelData,
                                int totalNumInputChannels,
                                float** outputChannelData,
                                int totalNumOutputChannels,
                                int numSamples) override;
    /** @internal */
    void audioDeviceAboutToStart (AudioIODevice*) override;
    /** @internal */
    void audioDeviceStopped() override;
    /** @internal */
    void handleIncomingMidiMessage (MidiInput* i, const MidiMessage&) override;

private:

    //==============================================================================
    GraphProcessor* processor;
    CriticalSection lock;
    double sampleRate;
    int blockSize;
    bool isPrepared;

    int numInputChans, numOutputChans;
    HeapBlock<float*> channels;
    AudioSampleBuffer tempBuffer;

    MidiBuffer incomingMidi;
    MidiMessageCollector messageCollector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphPlayer)
};

#endif /* ELEMENT_GRAPH_PLAYER_H */
