
#pragma once

#include "JuceHeader.h"

namespace Element
{

class LinearFade
{
public:
    LinearFade()
    {
        setSampleRate (44100.0);
        setFadesIn (true);
        setLength (1.0);
    }

    void setSampleRate (double newSampleRate)
    {
        jassert (newSampleRate > 0.0);
        sr = newSampleRate;
    }

    void reset()
    {
        envelope = fadesIn ? 0.0f : 1.0f;
        state = State::Idle;
    }

    void setLength (float newLength)
    {
        jassert (newLength > 0.f);
        length = newLength;
        updateFadeRate();
    }

    void setFadesIn (bool shouldFadeIn)
    {
        if (fadesIn == shouldFadeIn)
            return;
        fadesIn = shouldFadeIn;
        if (state == Idle)
            reset();
    }

    bool isActive() const noexcept { return state != State::Idle; }

    void startFading()
    {
        if (state == Idle)
            reset();
        state = State::Fading;
    }

    float getCurrentEnvelopeValue() const { return envelope; }

    float getNextEnvelopeValue()
    {
        if (state == State::Idle)
            return envelope;

        if (fadesIn)
        {
            envelope += fadeRate;
            if (envelope >= 1.0f)
            {
                envelope = 1.0f;
                state = Idle;
            };
        }
        else
        {
            envelope -= fadeRate;
            if (envelope <= 0.0f)
            {
                envelope = 0.0f;
                state = Idle;
            }
        }

        return envelope;
    }

private:
    void updateFadeRate()
    {
        // you haven't called setSampleRate() yet!
        jassert (sr > 0.0);
        fadeRate = (length > 0.0f ? static_cast<float> (1.0f / (length * sr)) : -1.0f);
    }

    enum State { Idle, Fading };
    State state = Idle;
    double sr { 0.0 };
    bool fadesIn { true };
    float length { 1.0f };
    float envelope { 0.f };
    float fadeRate { 0.f };
};

}
