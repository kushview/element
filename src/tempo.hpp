/*
    This file is part of the Kushview Modules for JUCE
    Copyright (c) 2014-2019  Kushview, LLC.  All rights reserved.

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

    uint8 getType() const { return static_cast<uint8> (type); }
    uint8 divisor() const { return (1 << type); }

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
