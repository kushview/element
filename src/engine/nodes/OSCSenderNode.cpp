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

#include "engine/nodes/OSCSenderNode.h"

namespace Element {

OSCSenderNode::OSCSenderNode()
    : MidiFilterNode (0)
{
    jassert (metadata.hasType (Tags::node));
    metadata.setProperty (Tags::format, "Element", nullptr);
    metadata.setProperty (Tags::identifier, EL_INTERNAL_ID_OSC_SENDER, nullptr);
}

OSCSenderNode::~OSCSenderNode()
{
   oscSender.disconnect();
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

void OSCSenderNode::render (AudioSampleBuffer& audio, MidiPipe& midi)
{
    auto timestamp = Time::getMillisecondCounterHiRes();
    const auto nframes = audio.getNumSamples();
    auto* const midiIn = midi.getWriteBuffer (0);

    if (nframes == 0 || !connected || paused) {
        midiIn->clear();
        return;
    }

    MidiBuffer::Iterator iter1 (*midiIn);
    MidiMessage msg;
    int frame;

    while (iter1.getNextEvent (msg, frame))
    {
        OSCMessage oscMsg = OSCProcessor::processMidiToOscMessage( msg );
        oscSender.send( oscMsg );

        if (oscMessages.size() > maxOscMessages)
            oscMessages.erase ( oscMessages.begin() );
        oscMessages.push_back ( oscMsg );
    }

    midiIn->clear();
}

/** For node editor */

bool OSCSenderNode::connect (String hostName, int portNumber)
{
    currentHostName = hostName;
    currentPortNumber = portNumber;

    connected = oscSender.connect (hostName, portNumber);
    return connected;
}

bool OSCSenderNode::disconnect ()
{
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

std::vector<OSCMessage> OSCSenderNode::getOscMessages()
{
    std::vector<OSCMessage> copied = oscMessages;
    oscMessages.clear();
    return copied;
}

}
