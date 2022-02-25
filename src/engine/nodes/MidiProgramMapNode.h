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

#include "engine/nodes/MidiFilterNode.h"
#include "engine/MidiPipe.h"
#include "engine/nodes/BaseProcessor.h"
#include "Signals.h"

namespace Element {

class MidiProgramMapNode : public MidiFilterNode,
                           public AsyncUpdater,
                           public ChangeBroadcaster
{
public:
    struct ProgramEntry
    {
        String name;
        int in;
        int out;
    };

    MidiProgramMapNode();
    virtual ~MidiProgramMapNode();

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_INTERNAL_ID_MIDI_PROGRAM_MAP;
        desc.name = "MIDI Program Map";
        desc.descriptiveName = "Filter MIDI Program Changes";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = "Element";
        desc.pluginFormatName = "Element";
        desc.version = "1.0.0";
        desc.uniqueId = EL_INTERNAL_UID_MIDI_PROGRAM_MAP;
    }

    void clear();

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;

    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void sendProgramChange (int program, int channel);
    int getNumProgramEntries() const;
    void addProgramEntry (const String& name, int programIn, int programOut = -1);
    void removeProgramEntry (int index);
    void editProgramEntry (int index, const String& name, int inProgram, int outProgram);
    ProgramEntry getProgramEntry (int index) const;

    inline int getWidth() const { return width; }
    inline int getHeight() const { return height; }

    inline void setSize (int w, int h)
    {
        width = jmax (w, (int) 1);
        height = jmax (h, (int) 1);
    }

    inline float getFontSize() const { return fontSize; }
    inline void setFontSize (float newSize)
    {
        fontSize = jlimit (9.f, 72.f, newSize);
    }

    inline int getLastProgram() const
    {
        ScopedLock sl (lock);
        return lastProgram;
    }

    void setState (const void* data, int size) override
    {
        const auto tree = ValueTree::readFromGZIPData (data, (size_t) size);
        if (! tree.isValid())
            return;

        clear();

        fontSize = jlimit (9.f, 72.f, (float) tree.getProperty ("fontSize", 15.f));
        width = jmax (10, (int) tree.getProperty ("width", 360));
        height = jmax (10, (int) tree.getProperty ("height", 540));

        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            const auto e = tree.getChild (i);
            auto* const entry = entries.add (new ProgramEntry());
            entry->name = e["name"].toString();
            entry->in = (int) e["in"];
            entry->out = (int) e["out"];
        }

        {
            ScopedLock sl (lock);
            for (const auto* const entry : entries)
                programMap[entry->in] = entry->out;
        }

        sendChangeMessage();
    }

    void getState (MemoryBlock& block) override
    {
        ValueTree tree ("state");
        tree.setProperty ("fontSize", fontSize, nullptr)
            .setProperty ("width", width, nullptr)
            .setProperty ("height", height, nullptr);
        for (const auto* const entry : entries)
        {
            ValueTree e ("entry");
            e.setProperty ("name", entry->name, nullptr)
                .setProperty ("in", entry->in, nullptr)
                .setProperty ("out", entry->out, nullptr);
            tree.appendChild (e, nullptr);
        }

        MemoryOutputStream stream (block, false);

        {
            GZIPCompressorOutputStream gzip (stream);
            tree.writeToStream (gzip);
        }
    }

    inline void handleAsyncUpdate() override { lastProgramChanged(); }
    Signal<void()> lastProgramChanged;

protected:
    CriticalSection lock;
    OwnedArray<ProgramEntry> entries;
    int programMap[128];

    bool assertedLowChannels = false;
    bool createdPorts = false;
    MidiBuffer* buffers[16];
    MidiBuffer tempMidi;
    MidiBuffer toSendMidi;

    int width = 360;
    int height = 540;
    float fontSize = 15.f;
    int lastProgram = -1;

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
};

} // namespace Element
