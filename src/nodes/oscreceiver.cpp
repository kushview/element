// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later
// Author: Eliot Akira <me@eliotakira.com>

#include "nodes/oscreceiver.hpp"
#include "utils.hpp"

namespace element {

OSCReceiverNode::OSCReceiverNode()
    : MidiFilterNode (0)
{
    setName ("OSC Receiver");
    oscReceiver.addListener (this);
}

OSCReceiverNode::~OSCReceiverNode()
{
    oscReceiver.removeListener (this);
    oscReceiver.disconnect();
}

void OSCReceiverNode::setState (const void* data, int size)
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
        connect (newPortNumber);

    currentHostName = newHostName;
    currentPortNumber = newPortNumber;
    connected = newConnected;
    paused = newPaused;

    sendChangeMessage();
}

void OSCReceiverNode::getState (MemoryBlock& block)
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

/** MIDI */

inline void OSCReceiverNode::refreshPorts()
{
    if (createdPorts)
        return;

    PortList newPorts;
    newPorts.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
    newPorts.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
    createdPorts = true;
    setPorts (newPorts);
}

void OSCReceiverNode::prepareToRender (double sampleRate, int maxBufferSize)
{
    if (! outputMidiMessagesInitDone)
    {
        outputMidiMessages.reset (sampleRate);
        currentSampleRate = sampleRate;
        outputMidiMessagesInitDone = true;
    }
}

void OSCReceiverNode::render (RenderContext& rc)
{
    const auto nframes = rc.audio.getNumSamples();
    if (nframes == 0)
        return;

    rc.midi.clear();
    outputMidiMessages.removeNextBlockOfMessages (*rc.midi.getWriteBuffer (0), nframes);
}

/** OSCReceiver real-time callbacks */

void OSCReceiverNode::oscMessageReceived (const OSCMessage& message)
{
    if (paused)
        return;

    auto timestamp = Time::getMillisecondCounterHiRes();

    MidiMessage midiMsg = Util::processOscToMidiMessage (message);
    midiMsg.setTimeStamp (timestamp);

    outputMidiMessages.addMessageToQueue (midiMsg);
};

void OSCReceiverNode::oscBundleReceived (const OSCBundle& bundle) {};

/** For node editor */

bool OSCReceiverNode::connect (int portNumber)
{
    if (connected && currentPortNumber == portNumber)
        return connected;

    currentPortNumber = portNumber;
    connected = oscReceiver.connect (portNumber);

    return connected;
}

bool OSCReceiverNode::disconnect()
{
    if (! connected)
        return true;
    connected = false;
    return oscReceiver.disconnect();
}

bool OSCReceiverNode::isConnected()
{
    return connected;
}

void OSCReceiverNode::pause()
{
    paused = true;
}

void OSCReceiverNode::resume()
{
    paused = false;
}

bool OSCReceiverNode::isPaused()
{
    return paused;
}

bool OSCReceiverNode::togglePause()
{
    if (paused)
        resume();
    else
        pause();

    return paused;
}

int OSCReceiverNode::getCurrentPortNumber()
{
    return currentPortNumber;
}

String OSCReceiverNode::getCurrentHostName()
{
    if (currentHostName == "")
        currentHostName = IPAddress::getLocalAddress().toString();
    return currentHostName;
}

void OSCReceiverNode::setPortNumber (int port)
{
    currentPortNumber = port;
}

void OSCReceiverNode::setHostName (String hostName)
{
    currentHostName = hostName;
}

/** OSCReceiver message loop callbacks */

void OSCReceiverNode::addMessageLoopListener (OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>* callback)
{
    oscReceiver.addListener (callback);
}

void OSCReceiverNode::removeMessageLoopListener (OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>* callback)
{
    oscReceiver.removeListener (callback);
}

} // namespace element
