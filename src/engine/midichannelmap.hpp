// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/juce.hpp>

namespace element {

class MidiChannelMap
{
public:
    MidiChannelMap()
    {
        tempMidi.ensureSize (sizeof (uint8) * 3 * 16);
        reset();
    }

    ~MidiChannelMap() {}

    inline void reset()
    {
        if (channelMap.size() <= 0)
            while (channelMap.size() <= 16)
                channelMap.add (0);
        for (int ch = 0; ch <= 16; ++ch)
            channelMap.getReference (ch) = ch;
    }

    inline void set (const int outputChan) noexcept
    {
        jassert (outputChan >= 1 && outputChan <= 16);
        for (int ch = 1; ch <= 16; ++ch)
            channelMap.getReference (ch) = outputChan;
    }

    inline void set (const int inputChan, const int outputChan) noexcept
    {
        jassert (inputChan >= 1 && inputChan <= 16 && outputChan >= 1 && outputChan <= 16);
        channelMap.getReference (inputChan) = outputChan;
    }

    inline int get (const int channel) const
    {
        jassert (channel >= 1 && channel <= 16);
        return channelMap.getUnchecked (channel);
    }

    inline void process (MidiMessage& message) const
    {
        if (message.getChannel() > 0)
            message.setChannel (channelMap.getUnchecked (message.getChannel()));
    }

    inline void render (MidiBuffer& midi)
    {
        for (auto m : midi)
        {
            auto msg = m.getMessage();
            process (msg);
            tempMidi.addEvent (msg, m.samplePosition);
        }

        midi.swapWith (tempMidi);
        tempMidi.clear();
    }

    const Array<int>& getMap() const { return channelMap; }

private:
    // TODO: optimize: use plain C array
    Array<int> channelMap;
    [[maybe_unused]] int channels[17];
    MidiBuffer tempMidi;
};

} // namespace element
