// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/midipipe.hpp>
#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/MidiFilterNode.h"
#include "engine/nodes/NodeTypes.h"

#include "semaphore.hpp"

namespace element {

class OSCSenderNode : public MidiFilterNode,
                      public ChangeBroadcaster,
                      public Thread
{
public:
    OSCSenderNode();
    virtual ~OSCSenderNode();

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.name = "OSC Sender";
        desc.fileOrIdentifier = EL_NODE_ID_OSC_SENDER;
        desc.uniqueId = EL_NODE_UID_OSC_SENDER;
        desc.descriptiveName = "OSC Sender";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    void getState (MemoryBlock& block) override;
    void setState (const void* data, int size) override;

    /** OSC -> MIDI processing thread */

    void run() override;
    void stop();

    /** MIDI */

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void releaseResources() override {};

    void refreshPorts() override;

    /** GUI */

    bool connect (String hostName, int portNumber);
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

    std::vector<OSCMessage> getOscMessages();

private:
    Semaphore sem;
    CriticalSection lock;

    /** MIDI */
    bool createdPorts = false;

    /** OSC */
    OSCSender oscSender;

    bool connected = false;
    bool paused = false;

    int currentPortNumber = 9002;
    String currentHostName = "127.0.0.1";

    /** Max messages in queue before they start getting dropped */
    int maxOscMessages = 100;

    /** GUI */
    std::vector<OSCMessage> oscMessagesToLog;

    /** To be processed and sent as OSC messages */
    MidiMessageCollector midiMessageQueue;

    Atomic<int> numSamples = 0;
    double currentSampleRate = 0;
    bool midiMessageQueueInitDone = false;
};

} // namespace element
