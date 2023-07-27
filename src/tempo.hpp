// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <cstdint>

namespace element {

struct BeatType
{
    enum ID
    {
        WholeNote = 0,
        HalfNote,
        QuarterNote,
        EighthNote,
        SixteenthNote
    };

    BeatType (const BeatType& o) { operator= (o); }
    BeatType (const ID& t) : type (t) {}

    inline static int fromDivisor (const int d)
    {
        switch (d)
        {
            case 1:
                return WholeNote;
                break;
            case 2:
                return HalfNote;
                break;
            case 4:
                return QuarterNote;
                break;
            case 8:
                return EighthNote;
                break;
            case 16:
                return SixteenthNote;
                break;
        }

        return QuarterNote;
        ;
    }

    inline static int fromPosition (const juce::AudioPlayHead::CurrentPositionInfo& info)
    {
        return fromDivisor (info.timeSigDenominator);
    }

    uint8_t getType() const { return static_cast<uint8_t> (type); }
    uint8_t divisor() const { return (1 << type); }

    operator int() const { return static_cast<int> (type); }
    operator ID() const { return type; }

    BeatType& operator= (const BeatType& o)
    {
        type = o.type;
        return *this;
    }

private:
    ID type;
};

class Tempo
{
public:
    /** Returns the number of audio frames per beat */
    inline static int audioFramesPerBeat (double sampleRate, double beatsPerMinute)
    {
        return juce::roundToInt (sampleRate * 60.0f / beatsPerMinute);
    }
};

} // namespace element
