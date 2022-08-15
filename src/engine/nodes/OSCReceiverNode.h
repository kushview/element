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

#pragma once

#include "engine/midipipe.hpp"
#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/MidiFilterNode.h"

namespace element {

class OSCReceiverNode : public MidiFilterNode,
                        public ChangeBroadcaster,
                        public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
{
public:
    OSCReceiverNode();
    virtual ~OSCReceiverNode();

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.name = "OSC Receiver";
        desc.fileOrIdentifier = EL_INTERNAL_ID_OSC_RECEIVER;
        desc.uniqueId = EL_INTERNAL_UID_OSC_RECEIVER;
        desc.descriptiveName = "OSC Receiver";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = "Element";
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    void refreshPorts() override;

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override {};
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void setState (const void* data, int size) override;
    void getState (MemoryBlock& block) override;

    /** For node editor */
    bool connect (int portNumber);
    bool disconnect();
    bool isConnected();
    void pause();
    void resume();
    bool togglePause();
    bool isPaused();
    int getCurrentPortNumber();
    String getCurrentHostName();
    void setPortNumber (int port);
    void setHostName (String hostName);

    void addMessageLoopListener (OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>* callback);
    void removeMessageLoopListener (OSCReceiver::Listener<OSCReceiver::MessageLoopCallback>* callback);

private:
    /** MIDI */
    bool createdPorts = false;
    double currentSampleRate;
    bool outputMidiMessagesInitDone = false;
    MidiMessageCollector outputMidiMessages;

    /** OSC */
    OSCReceiver oscReceiver;
    bool connected = false;
    bool paused = false;
    int currentPortNumber = 9001;
    String currentHostName = "";

    void oscMessageReceived (const OSCMessage& message) override;
    void oscBundleReceived (const OSCBundle& bundle) override;
};

} // namespace element
