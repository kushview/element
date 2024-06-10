// Copyright 2024 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/audio_basics.hpp>

namespace element {

/** 
    Set of parameters used when detecting if panic messages should be
    triggered.
 */
struct MidiPanicParams
{
    int channel { 0 }; ///> MIDI channel to allow detection on (zero is omni)
    int ccNumber { -1 }; ///> MIDI CC number. less than zero means 'disabled'
};

/** Collection of helpers to render panic messages. */
class MidiPanic
{
public:
    /** Write panic messags to a buffer. */
    inline static void write (juce::MidiBuffer& buffer, int ch, int frame)
    {
        buffer.addEvent (MidiMessage::allNotesOff (ch), frame);
        buffer.addEvent (MidiMessage::allSoundOff (ch), frame);
    }

    /** Write panic messages on all midi channels in the buffer. */
    inline static void write (juce::MidiBuffer& buffer, int frame)
    {
        for (int c = 1; c <= 16; ++c)
        {
            write (buffer, c, frame);
        }
    }

    /** 
        Returns a list of messages that can be used for "panicing".

        The time stamp of each message will be set to 
        `juce::Time::getMillisecondCounterHiRes` 
    */
    inline static std::vector<juce::MidiMessage> messages (int ch)
    {
        std::vector<juce::MidiMessage> msgs;
        const auto timestamp = Time::getMillisecondCounterHiRes();
        auto msg = MidiMessage::allNotesOff (ch);
        msg.setTimeStamp (timestamp);
        msgs.push_back (msg);
        msg = MidiMessage::allSoundOff (ch);
        msg.setTimeStamp (timestamp);
        msgs.push_back (msg);
        return msgs;
    }

    /** Create a vector of panic messages on on all midi channels. */
    inline static std::vector<juce::MidiMessage> messages()
    {
        std::vector<juce::MidiMessage> msgs;
        const auto timestamp = Time::getMillisecondCounterHiRes();
        for (int ch = 1; ch <= 16; ++ch)
        {
            auto msg = MidiMessage::allNotesOff (ch);
            msg.setTimeStamp (timestamp);
            msgs.push_back (msg);
            msg = MidiMessage::allSoundOff (ch);
            msg.setTimeStamp (timestamp);
            msgs.push_back (msg);
        }
        return msgs;
    }

    /** Replace the given CC messags with a panic set of messages.
      
        The input buffer is left unmodified. The out buffer will contain
        the original contents with the CC's replaced with panic messages.

        @param buffer Input midi
        @param out Output midi
        @param ccNumber The CC number to check.
     */
    inline static bool processCC (const juce::MidiBuffer& buffer, juce::MidiBuffer& out, int ccNumber, int channel)
    {
        bool processed = false;
        for (const auto r : buffer)
        {
            auto msg = r.getMessage();
            if (! (msg.isControllerOfType (ccNumber) && (channel == 0 || channel == msg.getChannel())))
            {
                out.addEvent (msg, r.samplePosition);
                continue;
            }

            if (! processed)
            {
                write (out, r.samplePosition);
                processed = true;
            }
        }

        return processed;
    }
};

} // namespace element
