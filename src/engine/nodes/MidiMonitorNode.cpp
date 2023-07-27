// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

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
