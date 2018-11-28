#pragma once

#include "JuceHeader.h"

namespace Element {

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
    inline static void process (MidiMessage& message, const int offset)
    {
        if (message.isNoteOnOrOff())
            message.setNoteNumber (offset + message.getNoteNumber());
    }

    /** Process a single event */
    inline void process (MidiMessage& message) noexcept 
    {
        if (message.isNoteOnOrOff())
            message.setNoteNumber (offset.get() + message.getNoteNumber());
    }

    /** Process a MidiBuffer */
    inline void process (MidiBuffer& midi, int numSamples)
    {
        if (0 == offset.get())
            return;

        MidiBuffer::Iterator iter (midi);
        MidiMessage msg; int frame = 0;
        
        while (iter.getNextEvent (msg, frame))
        {
            if (frame >= numSamples)
                break;
            if (msg.isNoteOnOrOff())
                msg.setNoteNumber (offset.get() + msg.getNoteNumber());
            output.addEvent (msg, frame);
        }

        midi.swapWith (output);
        output.clear();
    }

private:
    Atomic<int> offset { 0 };
    MidiBuffer output;
};

}
