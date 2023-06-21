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

#include <element/midipipe.hpp>
#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/MidiFilterNode.h"
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
        desc.fileOrIdentifier = EL_INTERNAL_ID_MIDI_MONITOR;
        desc.uniqueId = EL_INTERNAL_UID_MIDI_MONITOR;
        desc.descriptiveName = "MIDI Monitor";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_INTERNAL_FORMAT_AUTHOR;
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

    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;

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
