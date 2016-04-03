/*
    PortProcessor.h - This file is part of Element
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

#ifndef ELEMENT_PORTPROCESSOR_H
#define ELEMENT_PORTPROCESSOR_H

    class GraphProcessor;

//==============================================================================
/** A special type of Processor that can live inside an ProcessorGraph
    in order to use the audio that comes into and out of the graph itself.

    If you create an PortProcessor in "input" mode, it will act as a
    node in the graph which delivers the audio that is coming into the parent
    graph. This allows you to stream the data to other nodes and process the
    incoming audio.

    Likewise, one of these in "output" mode can be sent data which it will add to
    the sum of data being sent to the graph's output.

    @see AudioProcessorGraph
*/
class JUCE_API  GraphPort     : public Processor
{
public:

    //==============================================================================
    /** Returns the mode of this processor. */

    PortType type() const { return portType; }

    /** Returns the parent graph to which this processor belongs, or nullptr if it
        hasn't yet been added to one. */
    GraphProcessor* parent() const { return graph; }

    /** */
    uint32 getNumPorts();
    PortType getPortType (uint32 port) const;

    /** True if this is an audio or midi input. */
    bool isInput() const;

    /** True if this is an audio or midi output. */
    bool isOutput() const;

    //==============================================================================
    GraphPort (const PortType portType, bool isInputPort);
    ~GraphPort();

    void fillInPluginDescription (PluginDescription& d) const;

    const String getName() const;

    void prepareToPlay (double sampleRate, int estimatedSamplesPerBlock);
    void releaseResources();
    void processBlock (AudioSampleBuffer&, MidiBuffer&);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;
    bool acceptsMidi() const;
    bool producesMidi() const;
    bool hasEditor() const;
    AudioProcessorEditor* createEditor();
    int getNumParameters();
    const String getParameterName (int);
    float getParameter (int);
    const String getParameterText (int);
    void setParameter (int, float);

    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int);
    const String getProgramName (int);
    void changeProgramName (int, const String&);

    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

    /** @internal */
    void setGraph (GraphProcessor*);

private:

    const PortType  portType;
    GraphProcessor* graph;
    const bool portIsInput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphPort)

};

#endif // ELEMENT_PORTPROCESSOR_H
