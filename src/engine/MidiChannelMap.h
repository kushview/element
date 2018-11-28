#pragma once

#include "JuceHeader.h"

namespace Element {

class MidiChannelMap
{
public:
    MidiChannelMap()
    {
        tempMidi.ensureSize (sizeof(uint8) * 3 * 16);
        reset();
    }

    ~MidiChannelMap() { }

    inline void reset()
    {
        if (channelMap.size() <= 0)
            while (channelMap.size() <= 16)
                channelMap.add (0);
        for (int ch = 0; ch <= 16; ++ch)
            channelMap.getReference(ch) = ch;
    }

    inline void set (const int inputChan, const int outputChan) noexcept
    {
        jassert (inputChan >= 0 && inputChan <= 16 &&
                 outputChan >= 0 && outputChan <= 16);
        channelMap.getReference (inputChan) = outputChan;
    }

    inline void set (const int outputChan) noexcept
    {
        set (0, outputChan);
    }

    inline int get (const int channel) const
    {
        const auto omni = channelMap.getUnchecked (0);
        jassert (channel >= 0 && channel <= 16);
        jassert (omni >= 0 && omni <= 16);
        return channelMap.getUnchecked (omni > 0 ? omni : channel);
    }

    inline void process (MidiMessage& message) const
    {
        const auto channel = message.getChannel();
        const auto omni = channelMap.getUnchecked (0);
        jassert (channel >= 0 && channel <= 16);
        if (channel > 0)
            message.setChannel (channelMap.getUnchecked (omni > 0 ? omni : channel));
    }

    inline void render (MidiBuffer& midi)
    {
        MidiBuffer::Iterator iter (midi);
        MidiMessage msg; int frame = 0;
        
        while (iter.getNextEvent (msg, frame))
        {
            process (msg);
            tempMidi.addEvent (msg, frame);
        }

        midi.swapWith (tempMidi);
        tempMidi.clear();
    }

    const Array<int>& getMap() const { return channelMap; }

private:
    Array<int> channelMap;
    MidiBuffer tempMidi;
};

}
