// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/midipipe.hpp>
#include "nodes/baseprocessor.hpp"
#include "nodes/midifilter.hpp"
#include <element/signals.hpp>

namespace element {

class MidiMonitorNode : public MidiFilterNode,
                        private Timer
{
public:
    MidiMonitorNode();
    virtual ~MidiMonitorNode();

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.name = "MIDI Monitor";
        desc.fileOrIdentifier = EL_NODE_ID_MIDI_MONITOR;
        desc.uniqueId = EL_NODE_UID_MIDI_MONITOR;
        desc.descriptiveName = "MIDI Monitor";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    inline void refreshPorts() override
    {
        if (createdPorts)
            return;

        PortList newPorts;
        newPorts.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        newPorts.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
        createdPorts = true;
        setPorts (newPorts);
    }

    void clear() {}

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;

    void render (RenderContext& rc) override;

    void setState (const void* data, int size) override {};
    void getState (MemoryBlock& block) override {};

    void clearMessages();

    const StringArray& logger() const { return midiLog; }

private:
    friend class MidiMonitorNodeEditor;
    Signal<void()> messagesLogged;
    double currentSampleRate = 44100.0;
    int numSamples = 0;
    MidiMessageCollector inputMessages;
    bool createdPorts = false;
    CriticalSection lock;

    MidiBuffer midiTemp;
    StringArray midiLog;
    int maxLoggedMessages { 100 };
    float refreshRateHz { 60.0 };

    void getMessages (MidiBuffer& destBuffer);
    void timerCallback() override;
};

} // namespace element
