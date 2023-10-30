// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later
// Author: Eliot Akira <me@eliotakira.com>

#include "nodes/oscsender.hpp"
#include "utils.hpp"

namespace element {

OSCSenderNode::OSCSenderNode()
    : MidiFilterNode (0),
      Thread ("osc sender midi processing thread")
{
    setName ("OSC Sender");
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
        connect (newHostName, newPortNumber);

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

void OSCSenderNode::run()
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

        MidiMessage msg;
        ScopedLock sl (lock);

        for (auto m : messages)
        {
            msg = m.getMessage();
            OSCMessage oscMsg = Util::processMidiToOscMessage (msg);
            oscSender.send (oscMsg);

            if (! msg.isMidiClock())
            {
                oscMessagesToLog.push_back (oscMsg);
            }
        }

        while (oscMessagesToLog.size() > (size_t) maxOscMessages)
            oscMessagesToLog.erase (oscMessagesToLog.begin());
    }
}

void OSCSenderNode::stop()
{
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        sem.post();
        stopThread (100);
    }
}

/** MIDI */

inline void OSCSenderNode::refreshPorts()
{
    if (createdPorts)
        return;

    PortList newPorts;
    newPorts.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
    createdPorts = true;
    setPorts (newPorts);
}

void OSCSenderNode::prepareToRender (double sampleRate, int maxBufferSize)
{
    if (! midiMessageQueueInitDone)
    {
        midiMessageQueue.reset (sampleRate);
        currentSampleRate = sampleRate;
        midiMessageQueueInitDone = true;
    }
};

void OSCSenderNode::render (RenderContext& rc)
{
    const auto nframes = rc.audio.getNumSamples();
    auto* const midiIn = rc.midi.getWriteBuffer (0);

    if (nframes == 0 || ! connected || paused)
    {
        midiIn->clear();
        return;
    }

    MidiMessage msg;
    auto timestamp = Time::getMillisecondCounterHiRes();
    for (auto m : *midiIn)
    {
        msg = m.getMessage();
        msg.setTimeStamp (timestamp + (1000.0 * (static_cast<double> (m.samplePosition) / currentSampleRate)));
        midiMessageQueue.addMessageToQueue (msg);
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

bool OSCSenderNode::disconnect()
{
    if (! connected)
        return true;
    connected = false;
    return oscSender.disconnect();
}

bool OSCSenderNode::isConnected()
{
    return connected;
}

void OSCSenderNode::pause()
{
    paused = true;
}

void OSCSenderNode::resume()
{
    paused = false;
}

bool OSCSenderNode::isPaused()
{
    return paused;
}

bool OSCSenderNode::togglePause()
{
    if (paused)
        resume();
    else
        pause();

    return paused;
}

int OSCSenderNode::getCurrentPortNumber()
{
    return currentPortNumber;
}

String OSCSenderNode::getCurrentHostName()
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
        std::copy (oscMessagesToLog.begin(), oscMessagesToLog.end(), std::back_inserter (copied));
        oscMessagesToLog.clear();
    }

    return copied;
}

} // namespace element
