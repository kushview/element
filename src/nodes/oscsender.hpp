// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later
// Author: Eliot Akira <me@eliotakira.com>

#pragma once

#include <semaphore>

#include <element/midipipe.hpp>

#include "nodes/midifilter.hpp"
#include "nodetypes.hpp"

namespace element {

class OSCSenderNode : public MidiFilterNode,
                      public juce::ChangeBroadcaster,
                      public juce::Thread
{
public:
    OSCSenderNode();
    virtual ~OSCSenderNode();

    void getPluginDescription (juce::PluginDescription& desc) const override
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

    void getState (juce::MemoryBlock& block) override;
    void setState (const void* data, int size) override;

    /** OSC -> MIDI processing thread */

    void run() override;
    void stop();

    /** MIDI */

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void render (RenderContext&) override;
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

    std::vector<juce::OSCMessage> getOscMessages();

private:
    std::counting_semaphore<> sem { 0 };
    juce::CriticalSection lock;

    /** MIDI */
    bool createdPorts = false;

    /** OSC */
    juce::OSCSender oscSender;

    bool connected = false;
    bool paused = false;

    int currentPortNumber = 9002;
    juce::String currentHostName = "127.0.0.1";

    /** Max messages in queue before they start getting dropped */
    int maxOscMessages = 100;

    /** GUI */
    std::vector<juce::OSCMessage> oscMessagesToLog;

    /** To be processed and sent as OSC messages */
    juce::MidiMessageCollector midiMessageQueue;

    juce::Atomic<int> numSamples = 0;
    double currentSampleRate = 0;
    bool midiMessageQueueInitDone = false;
};

} // namespace element
