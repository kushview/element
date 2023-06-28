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

namespace element {

MidiMonitorNode::MidiMonitorNode()
    : MidiFilterNode (0)
{
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

    for (auto m : *midiIn)
    {
        auto msg = m.getMessage();
        msg.setTimeStamp (timestamp + (1000.0 * (static_cast<double> (m.samplePosition) / currentSampleRate)));
        inputMessages.addMessageToQueue (msg);
    }

    ScopedLock sl (lock);
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
    midiLog.clearQuick();
    {
        ScopedLock sl (lock);
        inputMessages.reset (currentSampleRate);
        numSamples = 0;
    }
    messagesLogged();
}

void MidiMonitorNode::timerCallback()
{
    midiTemp.clear();
    getMessages (midiTemp);
    if (midiTemp.getNumEvents() <= 0)
        return;

    int numLogged = 0;
    String text;
    for (auto m : midiTemp)
    {
        auto msg = m.getMessage();
        if (msg.isMidiClock())
        {
            text.clear();
            continue;
        }

        if (msg.isMidiStart())
            text << "Start";
        else if (msg.isMidiStop())
            text << "Stop";
        else if (msg.isMidiContinue())
            text << "Continue";

        if (msg.isNoteOn())
        {
            text << "NOTE ON: " << msg.getNoteNumber();
        }
        else if (msg.isNoteOff())
        {
            text << "NOTE OFF: " << msg.getNoteNumber();
        }

        midiLog.add (text.isNotEmpty() ? text : msg.getDescription());
        text.clear();
        ++numLogged;
    }

    if (midiLog.size() > maxLoggedMessages)
        midiLog.removeRange (0, midiLog.size() - maxLoggedMessages);

    if (numLogged > 0)
        messagesLogged();
}

}; // namespace element
