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

#include <cstdint>

#include <element/juce/audio_processors.hpp>
#include <element/timescale.hpp>

namespace element {

/** A mini-transport for use in a processable that can loop */
class Shuttle : public juce::AudioPlayHead
{
public:
    static const int PPQ;

    /** This function should be used when loading midi data.  e.g. opening
        a midi file that has a different ppq than the Shuttle */
    static double scaledTick (double sourceTick, const int srcPpq);

    struct Position : public juce::AudioPlayHead::CurrentPositionInfo
    {
        double timeInBeats;
    };

    Shuttle();
    ~Shuttle();

    bool isLooping() const;
    bool isPlaying() const;
    bool isRecording() const;

    double getFramesPerBeat() const;
    double getBeatsPerFrame() const;

    void setLengthBeats (const float beats);
    void setLengthFrames (const uint32_t df);
    void setLengthSeconds (const double seconds);

    const double getLengthBeats() const;
    const int64_t getLengthFrames() const;
    const double getLengthSeconds() const;

    const double getPositionBeats() const;
    const int64_t getPositionFrames() const;
    const double getPositionSeconds() const;

    int64_t getRemainingFrames() const;

    void resetRecording();

    const TimeScale& getTimeScale() const;
    float getTempo() const;
    void setTempo (float bpm);

    double getSampleRate() const;
    void setSampleRate (double rate);

    void advance (int nframes);
    inline void seekAudioFrame (int64_t frame)
    {
        framePos = frame;
    }

    bool getCurrentPosition (juce::AudioPlayHead::CurrentPositionInfo& result);
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override;

protected:
    TimeScale ts;
    bool playing, recording, looping;

private:
    double framesPerBeat;
    double beatsPerFrame;

    int64_t framePos;
    uint32_t duration;
    double sampleRate;

    double ppqLoopStart;
    double ppqLoopEnd;
};

} // namespace element
