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

#include "engine/nodes/OSCReceiverNode.h"

namespace Element {

OSCReceiverNode::OSCReceiverNode()
    : MidiFilterNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_OSC_RECEIVER, nullptr);

}

OSCReceiverNode::~OSCReceiverNode()
{
    // clearMessages();
}

void OSCReceiverNode::prepareToRender (double sampleRate, int maxBufferSize) {
/*    if (inputMessagesInitDone) {
        return;
    }
    inputMessages.reset (sampleRate);
    currentSampleRate = sampleRate;
    inputMessagesInitDone = true;*/
};

void OSCReceiverNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
/*    auto timestamp = Time::getMillisecondCounterHiRes();
    const auto nframes = audio.getNumSamples();

    if (nframes == 0) {
        return;
    }

    auto* const midiIn = midi.getWriteBuffer (0);
    MidiBuffer::Iterator iter1 (*midiIn);
    MidiMessage msg;
    int frame;

    while (iter1.getNextEvent (msg, frame))
    {
        // TODO: better timestamp sync with UI
        //       updating timestamp below causes messages to be skipped in the
        //       UI rendering
        // timestamp += 1000.0 * static_cast<double> (frame) * currentSampleRate;
        msg.setTimeStamp (timestamp);
        inputMessages.addMessageToQueue (msg);
    }

    numSamples += nframes;*/
}

void OSCReceiverNode::getMessages (MidiBuffer& destBuffer)
{
/*    inputMessages.removeNextBlockOfMessages (destBuffer, numSamples.get());
    numSamples = 0;*/
}

void OSCReceiverNode::clearMessages()
{
/*    inputMessages.reset (currentSampleRate);
    numSamples = 0;*/
}

};
