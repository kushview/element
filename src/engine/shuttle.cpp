// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/shuttle.hpp>
#include "tempo.hpp"

namespace element {

const int Shuttle::PPQ = 1920;

double Shuttle::scaledTick (double sourceTick, const int srcPpq)
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

juce::Optional<juce::AudioPlayHead::PositionInfo> Shuttle::getPosition() const
{
    juce::AudioPlayHead::PositionInfo info;
    info.setTimeInSamples (getPositionFrames());
    info.setTimeInSeconds (getPositionSeconds());
    info.setBpm ((double) ts.getTempo());
    juce::AudioPlayHead::TimeSignature timesig;
    timesig.numerator = ts.beatsPerBar();
    timesig.denominator = (1 << ts.beatDivisor());
    info.setTimeSignature (timesig);
    juce::AudioPlayHead::LoopPoints loops;
    loops.ppqEnd = 0.0;
    loops.ppqStart = 0.0;
    info.setLoopPoints (loops);

    {
        auto posBeats = getPositionBeats();
        info.setPpqPosition (posBeats);
        auto bar = static_cast<int64_t> (std::floor (posBeats / (float) ts.beatsPerBar()));
        info.setBarCount (bar);
        info.setPpqPositionOfLastBarStart (static_cast<double> (bar * ts.beatsPerBar()));
    }

    info.setEditOriginTime (0.0f);
    // info.setHostTimeNs();

    info.setFrameRate (AudioPlayHead::fps24);

    info.setIsPlaying (isPlaying());
    info.setIsRecording (isRecording());
    info.setIsLooping (isLooping());

    return info;
}

const double Shuttle::getLengthBeats() const { return getLengthSeconds() * (getTempo() / 60.0f); }
const int64_t Shuttle::getLengthFrames() const { return duration; }
const double Shuttle::getLengthSeconds() const { return (double) duration / (double) ts.getSampleRate(); }

const double Shuttle::getPositionBeats() const { return getPositionSeconds() * (getTempo() / 60.0f); }
const int64_t Shuttle::getPositionFrames() const { return framePos; }
const double Shuttle::getPositionSeconds() const { return (double) framePos / (double) ts.getSampleRate(); }

int64_t Shuttle::getRemainingFrames() const { return getLengthFrames() - framePos; }
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
void Shuttle::setLengthFrames (const uint32_t df) { duration = df; }

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
        duration = (uint32_t) llrint (oldLen * framesPerBeat);
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
    duration = (uint32_t) (oldLenSec * (float) ts.getSampleRate());
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
