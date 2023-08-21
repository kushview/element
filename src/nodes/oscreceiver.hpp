// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later
// Author: Eliot Akira <me@eliotakira.com>

#pragma once

#include <element/midipipe.hpp>
#include "nodes/baseprocessor.hpp"
#include "nodes/midifilter.hpp"

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
        desc.fileOrIdentifier = EL_NODE_ID_OSC_RECEIVER;
        desc.uniqueId = EL_NODE_UID_OSC_RECEIVER;
        desc.descriptiveName = "OSC Receiver";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
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
