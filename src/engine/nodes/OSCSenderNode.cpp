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

#include "engine/nodes/OSCSenderNode.h"
#include "Utils.h"

namespace Element {

OSCSenderNode::OSCSenderNode()
    : MidiFilterNode (0),
      Thread ("osc sender midi processing thread")
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_OSC_SENDER, nullptr);

    startThread();
}

OSCSenderNode::~OSCSenderNode()
{
    stop();
    oscSender.disconnect();
}

void OSCSenderNode::setState (const void* data, int size)
{
    const auto tree = ValueTree::readFromGZIPData (data, (size_t) size);
    if (! tree.isValid())
        return;

    String newHostName = tree.getProperty ("hostName", "").toString();
    int newPortNumber = jlimit (1, 65536, (int) tree.getProperty ("portNumber", 9001));
    bool newConnected = (bool) tree.getProperty ("connected", false);
    bool newPaused = (bool) tree.getProperty ("paused", false);

    if (newHostName != currentHostName || newPortNumber != currentPortNumber)
        disconnect();
    if (newConnected)
        connect(newHostName, newPortNumber);

    currentHostName = newHostName;
    currentPortNumber = newPortNumber;
    connected = newConnected;
    paused = newPaused;

    sendChangeMessage();
}

void OSCSenderNode::getState (MemoryBlock& block)
{
    ValueTree tree ("state");
    tree.setProperty ("hostName", currentHostName, nullptr);
    tree.setProperty ("portNumber", currentPortNumber, nullptr);
    tree.setProperty ("connected", connected, nullptr);
    tree.setProperty ("paused", paused, nullptr);

    MemoryOutputStream stream (block, false);

    {
        GZIPCompressorOutputStream gzip (stream);
        tree.writeToStream (gzip);
    }
}

void OSCSenderNode::run ()
{
    while (! threadShouldExit())
    {
        sem.wait();

        if (threadShouldExit())
            break;

        /** MIDI queue -> OSC messages */

        MidiBuffer messages;

        midiMessageQueue.removeNextBlockOfMessages (messages, numSamples.get());
        numSamples = 0;

        MidiBuffer::Iterator iter1 (messages);
        MidiMessage msg;
        int frame;

        ScopedLock sl (lock);

        while (iter1.getNextEvent (msg, frame))
        {
            OSCMessage oscMsg = Util::processMidiToOscMessage (msg);
            oscSender.send ( oscMsg );

            if (oscMessagesToLog.size() > maxOscMessages)
                oscMessagesToLog.erase ( oscMessagesToLog.begin() );
            oscMessagesToLog.push_back ( oscMsg );
        }
    }

    DBG("[EL] OSCSenderNode: OSC -> MIDI processing thread exited");
}

void OSCSenderNode::stop ()
{
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        sem.post();
        stopThread (100);
    }
}

/** MIDI */

inline void OSCSenderNode::createPorts()
{
    if (createdPorts)
        return;

    ports.clearQuick();

    ports.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
    ports.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
    createdPorts = true;
}

void OSCSenderNode::prepareToRender (double sampleRate, int maxBufferSize) {
    if (! midiMessageQueueInitDone) {
        midiMessageQueue.reset (sampleRate);
        currentSampleRate = sampleRate;
        midiMessageQueueInitDone = true;
    }
};

void OSCSenderNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    const auto nframes = audio.getNumSamples();
    auto* const midiIn = midi.getWriteBuffer (0);

    if (nframes == 0 || !connected || paused) {
        midiIn->clear();
        return;
    }

    MidiBuffer::Iterator iter1 (*midiIn);
    MidiMessage msg;
    int frame;
    auto timestamp = Time::getMillisecondCounterHiRes();

    while (iter1.getNextEvent (msg, frame))
    {
        msg.setTimeStamp ( timestamp + (1000.0 * (static_cast<double> (frame) / currentSampleRate)) );
        midiMessageQueue.addMessageToQueue ( msg );
    }

    numSamples += nframes;
    sem.post();
    midiIn->clear();
}

/** For node editor */

bool OSCSenderNode::connect (String hostName, int portNumber)
{
   if (connected && currentPortNumber == portNumber)
        return connected;

    currentHostName = hostName;
    currentPortNumber = portNumber;
    connected = oscSender.connect (hostName, portNumber);

    return connected;
}

bool OSCSenderNode::disconnect ()
{
   if (!connected)
        return true;
    connected = false;
    return oscSender.disconnect();
}

bool OSCSenderNode::isConnected ()
{
    return connected;
}

void OSCSenderNode::pause ()
{
    paused = true;
}

void OSCSenderNode::resume ()
{
    paused = false;
}

bool OSCSenderNode::isPaused ()
{
    return paused;
}

bool OSCSenderNode::togglePause ()
{
    if ( paused )
        resume();
    else
        pause();

    return paused;
}

int OSCSenderNode::getCurrentPortNumber ()
{
    return currentPortNumber;
}

String OSCSenderNode::getCurrentHostName ()
{
    return currentHostName;
}

void OSCSenderNode::setPortNumber (int port)
{
    currentPortNumber = port;
}

void OSCSenderNode::setHostName (String hostName)
{
    currentHostName = hostName;
}

std::vector<OSCMessage> OSCSenderNode::getOscMessages()
{
    std::vector<OSCMessage> copied;

    {
        ScopedLock sl (lock);
        std::copy ( oscMessagesToLog.begin(), oscMessagesToLog.end(), std::back_inserter( copied ) );
        oscMessagesToLog.clear();
    }

    return copied;
}

}
