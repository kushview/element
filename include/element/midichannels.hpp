// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/juce/core.hpp>

namespace element {

struct JUCE_API MidiChannels {
    inline MidiChannels()
    {
        reset();
    }

    inline MidiChannels (int channel)
    {
        reset();
        if (channel <= 0)
            setOmni (true);
        else
            setChannel (channel);
    }

    inline MidiChannels (const MidiChannels& o) { operator= (o); }

    ~MidiChannels() = default;

    /** Reset channels and toggle omni to on */
    inline void reset()
    {
        channels.setRange (0, 17, false);
        channels.setBit (0, true);
    }

    /** Set a specific channel */
    inline void setChannel (const int channel)
    {
        jassert (channel >= 1 && channel <= 16);
        reset();
        channels.setBit (0, false);
        channels.setBit (channel, true);
    }

    /** Set channels */
    inline void setChannels (const juce::BigInteger newChannels)
    {
        channels = newChannels;
    }

    /** Set if omni */
    inline void setOmni (const bool omni) { channels.setBit (0, omni); }

    /** Returns true if it is omni */
    inline bool isOmni() const noexcept { return channels[0]; }

    /** Returns true if omni or the channel is toggled */
    inline bool isOn (const int channel) const noexcept { return channels[0] || channels[channel]; }

    /** Returns true if the channel isn't omni and isn't toggled */
    inline bool isOff (const int channel) const noexcept { return ! channels[0] && ! channels[channel]; }

    inline MidiChannels& operator= (const MidiChannels& o)
    {
        channels = o.channels;
        return *this;
    }

    /** Returns the underlying juce::BigInteger */
    const juce::BigInteger& get() const { return channels; }

private:
    juce::BigInteger channels;
};

} // namespace element
