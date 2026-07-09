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
    tempo = 120.0f;
    sampleRate = 44100.0;
    beatsPerBar = 4;
    beatType = 2;
    beatDivisor = 2;

    duration = 0;
    framePos = 0;
    framesPerBeat = Tempo::audioFramesPerBeat (sampleRate, tempo);
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
    info.setBpm ((double) tempo);
    juce::AudioPlayHead::TimeSignature timesig;
    timesig.numerator = beatsPerBar;
    timesig.denominator = (1 << beatDivisor);
    info.setTimeSignature (timesig);
    juce::AudioPlayHead::LoopPoints loops;
    loops.ppqEnd = 0.0;
    loops.ppqStart = 0.0;
    info.setLoopPoints (loops);

    {
        auto posBeats = getPositionBeats();
        info.setPpqPosition (posBeats);
        auto bar = static_cast<int64_t> (std::floor (posBeats / (float) beatsPerBar));
        info.setBarCount (bar);
        info.setPpqPositionOfLastBarStart (static_cast<double> (bar * beatsPerBar));
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
const double Shuttle::getLengthSeconds() const { return (double) duration / sampleRate; }

const double Shuttle::getPositionBeats() const { return getPositionSeconds() * (getTempo() / 60.0f); }
const int64_t Shuttle::getPositionFrames() const { return framePos; }
const double Shuttle::getPositionSeconds() const { return (double) framePos / sampleRate; }

int64_t Shuttle::getRemainingFrames() const { return getLengthFrames() - framePos; }
double Shuttle::getSampleRate() const { return sampleRate; }
float Shuttle::getTempo() const { return tempo; }

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
    if (tempo != bpm && bpm > 0.0f)
    {
        double oldTime = getPositionBeats();
        double oldLen = getLengthBeats();

        tempo = bpm;
        framesPerBeat = (double) Tempo::audioFramesPerBeat (sampleRate, tempo);

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
    sampleRate = rate;

    framePos = llrint (oldTime * sampleRate);
    duration = (uint32_t) (oldLenSec * (float) sampleRate);
    framesPerBeat = Tempo::audioFramesPerBeat (sampleRate, tempo);
    beatsPerFrame = 1.0f / framesPerBeat;
}

void Shuttle::advance (int nframes)
{
    framePos += nframes;
    if (duration > 0 && framePos >= duration)
        framePos = framePos - duration;
}

} // namespace element
