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

#pragma once

#include "engine/MidiPipe.h"
#include "engine/nodes/BaseProcessor.h"
#include "engine/nodes/MidiFilterNode.h"
#include "Signals.h"

namespace Element {

class OSCReceiverNode   : public MidiFilterNode,
                          public AsyncUpdater,
                          public ChangeBroadcaster
{
public:
    OSCReceiverNode();
    virtual ~OSCReceiverNode();

    void fillInPluginDescription (PluginDescription& desc)
    {
        desc.name               = "OSC Receiver";
        desc.fileOrIdentifier   = EL_INTERNAL_ID_OSC_RECEIVER;
        desc.uid                = EL_INTERNAL_UID_OSC_RECEIVER;
        desc.descriptiveName    = "OSC Receiver";
        desc.numInputChannels   = 0;
        desc.numOutputChannels  = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument       = false;
        desc.manufacturerName   = "Element";
        desc.pluginFormatName   = "Element";
        desc.version            = "1.0.0";
    }

    void clear() {};

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override {};

    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;

    void setState (const void* data, int size) override {};
    void getState (MemoryBlock& block) override {};

    inline void handleAsyncUpdate() override {};

    void clearMessages();
    void getMessages(MidiBuffer &destBuffer);

private:
//    friend class OSCReceiverNodeEditor;
    bool inputMessagesInitDone = false;
    double currentSampleRate;
    Atomic<int> numSamples = 0;
    MidiMessageCollector inputMessages;
    bool createdPorts = false;

    inline void createPorts() override
    {
        if (createdPorts)
            return;

        ports.clearQuick();
        //ports.add (PortType::Midi, 0, 0, "midi_in", "MIDI In", true);
        ports.add (PortType::Midi, 1, 0, "midi_out", "MIDI Out", false);
        createdPorts = true;
    }
};

}