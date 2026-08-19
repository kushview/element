// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "nodes/midimonitor.hpp"
#include "utils.hpp"

namespace element {

MidiMonitorNode::MidiMonitorNode()
    : MidiFilterNode (0)
{
    setName ("MIDI Monitor");
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

void MidiMonitorNode::render (RenderContext& rc)
{
    auto timestamp = Time::getMillisecondCounterHiRes();
    const auto nframes = rc.audio.getNumSamples();

    if (nframes == 0)
        return;

    auto* const midiIn = rc.midi.getWriteBuffer (0);

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

String MidiMonitorNode::describe (const MidiMessage& msg)
{
    if (msg.isMidiClock())
        return {};

    if (msg.isMidiStart())
        return "Start";
    if (msg.isMidiStop())
        return "Stop";
    if (msg.isMidiContinue())
        return "Continue";

    if (msg.isNoteOn() || msg.isNoteOff())
    {
        String text;
        text << (msg.isNoteOn() ? "Note On " : "Note Off ")
             << Util::noteValueToString (msg.getNoteNumber())
             << " (" << msg.getNoteNumber() << ")"
             << " Velocity " << (int) msg.getVelocity()
             << " Channel " << msg.getChannel();
        return text;
    }

    return msg.getDescription();
}

void MidiMonitorNode::timerCallback()
{
    midiTemp.clear();
    getMessages (midiTemp);
    if (midiTemp.getNumEvents() <= 0)
        return;

    int numLogged = 0;
    for (auto m : midiTemp)
    {
        const auto text = describe (m.getMessage());
        if (text.isEmpty())
            continue;

        midiLog.add (text);
        ++numLogged;
    }

    if (midiLog.size() > maxLoggedMessages)
        midiLog.removeRange (0, midiLog.size() - maxLoggedMessages);

    if (numLogged > 0)
        messagesLogged();
}

}; // namespace element
