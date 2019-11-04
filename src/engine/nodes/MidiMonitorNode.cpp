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

#include "engine/nodes/MidiMonitorNode.h"

namespace Element {

MidiMonitorNode::MidiMonitorNode()
    : MidiFilterNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_MIDI_MONITOR, nullptr);

}

MidiMonitorNode::~MidiMonitorNode()
{
    clearMessages();
}

void MidiMonitorNode::prepareToRender (double sampleRate, int maxBufferSize) {
    inputMessages.reset(sampleRate);
    currentSampleRate = sampleRate;
};

void MidiMonitorNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    ignoreUnused (audio, midi);

    int numBuffers = midi.getNumBuffers();
    if (numBuffers <= 0)
    {
        return;
    }

    auto* const midiIn = midi.getWriteBuffer (0);
    MidiBuffer::Iterator iter1 (*midiIn);
    MidiMessage msg;
    int frame;

    while (iter1.getNextEvent (msg, frame)) {
        numSamples += frame;
        inputMessages.addMessageToQueue(msg);
    }
}

void MidiMonitorNode::getMessages(MidiBuffer &destBuffer) {
    inputMessages.removeNextBlockOfMessages(destBuffer, numSamples);
    numSamples = 0;
}

void MidiMonitorNode::clearMessages() {
    inputMessages.reset(currentSampleRate);
    numSamples = 0;
}

};