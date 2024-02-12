// Copyright 2024 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "nodes/midifilter.hpp"
#include <element/node.h>
#include <element/midipipe.hpp>
#include <element/signals.hpp>

namespace element {

class Context;

class MidiSetListProcessor : public MidiFilterNode,
                           public AsyncUpdater,
                           public ChangeBroadcaster
{
public:
    struct ProgramEntry
    {
        String name;
        int in;
        int out;
        double tempo { 0.0 };
    };

    MidiSetListProcessor (Context&);
    virtual ~MidiSetListProcessor();

    void getPluginDescription (PluginDescription& desc) const override
    {
        desc.fileOrIdentifier = EL_NODE_ID_MIDI_SET_LIST;
        desc.name = getName();
        desc.descriptiveName = "Filter MIDI Programs and change tempo.";
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.hasSharedContainer = false;
        desc.isInstrument = false;
        desc.manufacturerName = EL_NODE_FORMAT_AUTHOR;
        desc.pluginFormatName = EL_NODE_FORMAT_NAME;
        desc.version = "1.0.0";
        desc.uniqueId = EL_NODE_UID_MIDI_SET_LIST;
    }

    void clear();

    void prepareToRender (double sampleRate, int maxBufferSize) override;
    void releaseResources() override;

    void render (RenderContext&) override;
    void sendProgramChange (int program, int channel);

    int getNumProgramEntries() const;
    void addProgramEntry (const String& name, int programIn, int programOut = -1);
    void removeProgramEntry (int index);
    void editProgramEntry (int index,
                           const String& name,
                           int inProgram,
                           int outProgram,
                           double tempo);
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
            entry->tempo = (double) e["tempo"];
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
                .setProperty ("out", entry->out, nullptr)
                .setProperty ("tempo", entry->tempo, nullptr);
            tree.appendChild (e, nullptr);
        }

        MemoryOutputStream stream (block, false);

        {
            GZIPCompressorOutputStream gzip (stream);
            tree.writeToStream (gzip);
        }
    }

    void handleAsyncUpdate() override;
    Signal<void()> lastProgramChanged;

protected:
    Context& _context;
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

    void maybeSendTempo (int program);
};

} // namespace element
