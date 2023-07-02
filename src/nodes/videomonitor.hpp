/*
    This file is part of Element
    Copyright (C) 2023  Kushview, LLC.  All rights reserved.

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

#include <element/ui/nodeeditor.hpp>
#include <element/ui/style.hpp>
#include <element/processor.hpp>
#include <element/porttype.hpp>

#include "engine/nodes/NodeTypes.h"

namespace element {

class VideoMonitorNode : public Processor,
                     public ChangeBroadcaster
{
public:
    VideoMonitorNode()
        : Processor (PortCount().with (PortType::Midi, 1, 0).with (PortType::Control, 0, 9).toPortList()) {}
    ~VideoMonitorNode() {}

    /** Open the device. e.g. go online. */
    void open()
    {
        uint8_t data[5] = { 0x00, 0x00, 0x66, 0x14, 0x00 };
        data[4] = 0x63;

        sendMidi (MidiMessage::createSysExMessage (data, 5));
    }

    void close()
    {
    }

    void sendMidi (const MidiMessage& msg)
    {
        col.addMessageToQueue (msg.withTimeStamp ((double) Time::getMillisecondCounter() / 1000.0));
    }

    //==========================================================================
    void prepareToRender (double sampleRate, int maxBufferSize) override
    {
        ignoreUnused (sampleRate, maxBufferSize);
        col.reset (sampleRate);
    }

    void releaseResources() override {}

    inline bool wantsMidiPipe() const override { return true; }
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override
    {
        auto buf = midi.getWriteBuffer (0);
        for (const auto msg : *buf)
        {
            juce::ignoreUnused (msg);
        }
        midi.clear();
        col.removeNextBlockOfMessages (*buf, audio.getNumSamples());
    }

    void getState (MemoryBlock&) override {}
    void setState (const void*, int sizeInBytes) override {}

    CriticalSection& getLock() { return lock; }

    int getNumPrograms() const override { return 1; }
    int getCurrentProgram() const override { return 0; }
    void setCurrentProgram (int index) override {}
    const String getProgramName (int index) const override
    {
        return "MCU";
    }

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_NODE_ID_MCU;
        desc.uniqueId = EL_NODE_UID_MCU;
        desc.name = "MCU Brain";
        desc.descriptiveName = "Support for Mackie Control Universal";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
    }

    void refreshPorts() override
    {
        // nothing to do yet
    }

private:
    CriticalSection lock;
    juce::MidiMessageCollector col;
};

class MackieControlEditor : public NodeEditor
{
public:
    MackieControlEditor (const Node& node)
        : NodeEditor (node)
    {
        setOpaque (true);
        addAndMakeVisible (testButton);
        testButton.setButtonText ("Test");
        testButton.onClick = [this]() {
            proc()->sendMidi (MidiMessage::noteOn (1, 100, 1.f));
            proc()->sendMidi (MidiMessage::noteOff (1, 100));
        };

        addAndMakeVisible (onlineButton);
        onlineButton.setButtonText ("Reset");
        onlineButton.setClickingTogglesState (true);
        onlineButton.setToggleState (false, dontSendNotification);
        onlineButton.onClick = [this]() {
            if (onlineButton.getToggleState())
            {
                proc()->open();
            }
            else
            {
                proc()->close();
            }
        };

        addAndMakeVisible (fader);
        fader.setRange (0.0, 16383.0, 1.0);
        fader.setSliderStyle (Slider::LinearVertical);
        fader.onValueChange = [this]() {
            proc()->sendMidi (MidiMessage::pitchWheel (1, roundToInt (fader.getValue())));
        };

        setSize (300, 500);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (20);
        testButton.setBounds (r.removeFromTop (24));
        onlineButton.setBounds (r.removeFromTop (24));
        fader.setBounds (r.removeFromLeft (20));
    }

    void paint (Graphics& g) override
    {
        g.fillAll (element::Colors::widgetBackgroundColor);
    }

private:
    TextButton testButton;
    TextButton onlineButton;
    Slider fader;

    VideoMonitorNode* proc() const noexcept { return getNodeObjectOfType<VideoMonitorNode>(); }
};

} // namespace element
