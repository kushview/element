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

#include "engine/shuttle.hpp"
#include "tempo.hpp"

namespace element {

const int32 Shuttle::PPQ = 1920;

double Shuttle::scaledTick (double sourceTick, const int32 srcPpq)
{
    if (srcPpq == Shuttle::PPQ || srcPpq <= 0)
        return sourceTick;

    return sourceTick * ((double) Shuttle::PPQ / (double) srcPpq);
}

Shuttle::Shuttle()
{
    ts.setTempo (120.0f);
    ts.setSampleRate (44100);
    ts.setTicksPerBeat (Shuttle::PPQ);
    ts.updateScale();

    duration = 0;
    framePos = 0;
    framesPerBeat = Tempo::audioFramesPerBeat ((double) ts.getSampleRate(), ts.getTempo());
    beatsPerFrame = 1.0f / framesPerBeat;
    playing = recording = false;
    looping = true;
}

Shuttle::~Shuttle() {}

double Shuttle::getBeatsPerFrame() const { return beatsPerFrame; }
double Shuttle::getFramesPerBeat() const { return framesPerBeat; }

bool Shuttle::getCurrentPosition (CurrentPositionInfo& result)
{
    result.bpm = (double) ts.getTempo();
    result.frameRate = AudioPlayHead::fps24;

    result.isLooping = isLooping();
    result.isPlaying = isPlaying();
    result.isRecording = isRecording();

    result.ppqLoopStart = 0.0f; // ppqLoopStart;
    result.ppqLoopEnd = 0.0f; // ppqLoopEnd;
    result.ppqPosition = getPositionBeats();
    result.ppqPositionOfLastBarStart = 0.0f;

    result.editOriginTime = 0.0f;

    result.timeInSamples = getPositionFrames();
    result.timeInSeconds = getPositionSeconds();
    result.timeSigNumerator = ts.beatsPerBar();
    result.timeSigDenominator = (1 << ts.beatDivisor());

    return true;
}

juce::Optional<juce::AudioPlayHead::PositionInfo> Shuttle::getPosition() const
{
    juce::AudioPlayHead::PositionInfo info;
    info.setBpm ((double) ts.getTempo());
    info.setFrameRate (AudioPlayHead::fps24);

    info.setIsLooping (isLooping());
    info.setIsPlaying (isPlaying());
    info.setIsRecording (isRecording());

    // juce::AudioPlayHead::LoopPoints loops;
    // info.setLoopPoints ()
    // info.ppqLoopEnd   = 0.0f; // ppqLoopEnd;
    info.setPpqPosition (getPositionBeats());
    info.setPpqPositionOfLastBarStart (0.0f);

    info.setEditOriginTime (0.0f);

    juce::AudioPlayHead::TimeSignature timesig;
    info.setTimeInSamples (getPositionFrames());
    info.setTimeInSeconds (getPositionSeconds());
    timesig.numerator = ts.beatsPerBar();
    timesig.denominator = (1 << ts.beatDivisor());
    info.setTimeSignature (timesig);
    return info;
}

const double Shuttle::getLengthBeats() const { return getLengthSeconds() * (getTempo() / 60.0f); }
const int64 Shuttle::getLengthFrames() const { return duration; }
const double Shuttle::getLengthSeconds() const { return (double) duration / (double) ts.getSampleRate(); }

const double Shuttle::getPositionBeats() const { return getPositionSeconds() * (getTempo() / 60.0f); }
const int64 Shuttle::getPositionFrames() const { return framePos; }
const double Shuttle::getPositionSeconds() const { return (double) framePos / (double) ts.getSampleRate(); }

int64 Shuttle::getRemainingFrames() const { return getLengthFrames() - framePos; }
double Shuttle::getSampleRate() const { return (double) ts.getSampleRate(); }
float Shuttle::getTempo() const { return ts.getTempo(); }
const TimeScale& Shuttle::getTimeScale() const { return ts; }

bool Shuttle::isLooping() const { return looping; }
bool Shuttle::isPlaying() const { return playing; }
bool Shuttle::isRecording() const { return recording; }

void Shuttle::resetRecording()
{
    // TODO:
}

void Shuttle::setLengthBeats (const float beats) { setLengthFrames (framesPerBeat * beats); }
void Shuttle::setLengthSeconds (const double seconds) { setLengthFrames (juce::roundToInt (getSampleRate() * seconds)); }
void Shuttle::setLengthFrames (const uint32 df) { duration = df; }

void Shuttle::setTempo (float bpm)
{
    if (ts.getTempo() != bpm && bpm > 0.0f)
    {
        double oldTime = getPositionBeats();
        double oldLen = getLengthBeats();

        ts.setTempo (bpm);
        ts.updateScale();
        framesPerBeat = (double) Tempo::audioFramesPerBeat ((double) ts.getSampleRate(), ts.getTempo());

        beatsPerFrame = 1.0f / framesPerBeat;
        framePos = llrint (oldTime * framesPerBeat);
        duration = (uint32) llrint (oldLen * framesPerBeat);
    }
}

void Shuttle::setSampleRate (double rate)
{
    if (sampleRate == rate)
        return;

    const double oldTime = getPositionSeconds();
    const double oldLenSec = (double) getLengthSeconds();
    ts.setSampleRate (rate);
    ts.updateScale();

    framePos = llrint (oldTime * ts.getSampleRate());
    duration = (uint32) (oldLenSec * (float) ts.getSampleRate());
    framesPerBeat = Tempo::audioFramesPerBeat (ts.getSampleRate(), ts.getTempo());
    beatsPerFrame = 1.0f / framesPerBeat;
}

void Shuttle::advance (int nframes)
{
    framePos += nframes;
    if (duration > 0 && framePos >= duration)
        framePos = framePos - duration;
}

} // namespace element
