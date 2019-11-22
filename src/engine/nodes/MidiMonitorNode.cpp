/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.
    Author Eliot Akira <me@eliotakira.com>
    
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
    midiTemp.ensureSize (3 * 32);
}

MidiMonitorNode::~MidiMonitorNode()
{
    stopTimer();
    clearMessages();
}

void MidiMonitorNode::prepareToRender (double sampleRate, int maxBufferSize)
{
    inputMessages.reset (sampleRate);
    currentSampleRate = sampleRate;
    numSamples = 0;
    startTimerHz (refreshRateHz);
};

void MidiMonitorNode::releaseResources()
{
    stopTimer();
}

void MidiMonitorNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    auto timestamp = Time::getMillisecondCounterHiRes();
    const auto nframes = audio.getNumSamples();

    if (nframes == 0)
        return;

    auto* const midiIn = midi.getWriteBuffer (0);
    MidiBuffer::Iterator iter (*midiIn);
    MidiMessage msg;
    int frame;

    ScopedLock sl (lock);
    while (iter.getNextEvent (msg, frame))
    {
        msg.setTimeStamp (timestamp + (1000.0 * (static_cast<double> (frame) / currentSampleRate)));
        inputMessages.addMessageToQueue (msg);
    }

    numSamples += nframes;
}

void MidiMonitorNode::getMessages (MidiBuffer& destBuffer)
{
    ScopedLock sl (lock);
    if (numSamples <= 0)
        return;
    inputMessages.removeNextBlockOfMessages (destBuffer, numSamples);
    numSamples = 0;
}

void MidiMonitorNode::clearMessages()
{
    inputMessages.reset (currentSampleRate);
    numSamples = 0;
}

void MidiMonitorNode::timerCallback()
{
    midiTemp.clear();
    getMessages (midiTemp);
    if (midiTemp.getNumEvents() <= 0)
        return;

    MidiBuffer::Iterator iter (midiTemp);
    MidiMessage msg; int frame = 0;

    while (iter.getNextEvent (msg, frame))
        midiLog.add (msg.getDescription());

    if (midiLog.size() > maxLoggedMessages)
        midiLog.removeRange (0, midiLog.size() - maxLoggedMessages);
    
    messagesLogged();
}

};
