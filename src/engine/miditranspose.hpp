// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>

namespace element {

class MidiTranspose
{
public:
    MidiTranspose()
    {
        output.ensureSize (sizeof (uint8) * 3 * 16);
    }

    ~MidiTranspose()
    {
        output.clear();
    }

    /** Set the note offset to transpose by. e.g -12 is down one octave */
    inline void setNoteOffset (const int noteOffset) { offset.set (noteOffset); }

    /** Returns the current note offset */
    inline int getNoteOffset() const { return offset.get(); }

    /** Process a single event */
    inline static void process (juce::MidiMessage& message, const int offset)
    {
        if (message.isNoteOnOrOff())
            message.setNoteNumber (offset + message.getNoteNumber());
    }

    /** Process a single event */
    inline void process (juce::MidiMessage& message) noexcept
    {
        if (message.isNoteOnOrOff())
            message.setNoteNumber (offset.get() + message.getNoteNumber());
    }

    /** Process a MidiBuffer */
    inline void process (juce::MidiBuffer& midi, int numSamples)
    {
        if (0 == offset.get())
            return;

        juce::MidiMessage msg;
        for (auto m : midi)
        {
            if (m.samplePosition >= numSamples)
                break;
            msg = m.getMessage();
            if (msg.isNoteOnOrOff())
                msg.setNoteNumber (offset.get() + msg.getNoteNumber());
            output.addEvent (msg, m.samplePosition);
        }

        midi.swapWith (output);
        output.clear();
    }

private:
    juce::Atomic<int> offset { 0 };
    juce::MidiBuffer output;
};

} // namespace element
