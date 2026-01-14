// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"

namespace element {

class VelocityCurve
{
public:
    enum Mode
    {
        Linear = 0,
        Soft_1,
        Soft_2,
        Soft_3,
        Hard_1,
        Hard_2,
        Hard_3,
        Max,
        numModes
    };

    VelocityCurve() { setMode (Linear); }
    ~VelocityCurve() {}

    inline static String getModeName (const int mode)
    {
        switch (mode)
        {
            case Linear:
                return "Linear";
                break;
            case Soft_1:
                return "Soft";
                break;
            case Soft_2:
                return "Softer";
                break;
            case Soft_3:
                return "Softest";
                break;
            case Hard_1:
                return "Hard";
                break;
            case Hard_2:
                return "Harder";
                break;
            case Hard_3:
                return "Hardest";
                break;
            case Max:
                return "Max";
                break;
        }

        return {};
    }

    inline String getModeName() const { return getModeName (mode); }
    inline int getMode() const { return static_cast<int> (mode); }
    inline void setMode (const Mode m)
    {
        if (mode == m)
            return;
        mode = m;

        switch (mode)
        {
            default:
            case Max:
            case Linear:
                setOffset (0.50f);
                break;
            case Soft_1:
                setOffset (0.45f);
                break;
            case Soft_2:
                setOffset (0.35f);
                break;
            case Soft_3:
                setOffset (0.25f);
                break;
            case Hard_1:
                setOffset (0.55f);
                break;
            case Hard_2:
                setOffset (0.65f);
                break;
            case Hard_3:
                setOffset (0.75f);
                break;
        }
    }

    inline float process (float velocity)
    {
        if (mode == Linear)
        {
            return velocity;
        }
        else if (mode == Max)
        {
            return 1.f;
        }

        jassert (velocity >= 0 && velocity <= 1.f);
        jassert (mode >= Soft_1 && mode <= Hard_3);

        if (t < 0.5)
        {
            velocity = c1 - sqrtf (rsq - square (127.f * velocity - c0));
        }
        else if (t > 0.5)
        {
            velocity = c1 + sqrtf (rsq - square (127.f * velocity - c0));
        }
        else
        {
            jassertfalse;
        }

        return velocity / 127.f;
    }

    inline uint8 process (const uint8 velocity)
    {
        if (mode == Linear)
            return velocity;
        else if (mode == Max)
            return 127;
        return static_cast<uint8> (roundToIntAccurate (
            process (static_cast<float> (velocity) / 127.f)));
    }

private:
    Mode mode;
    float rsq;
    float c0, c1;
    float t;

    void setOffset (float newT)
    {
#define dbgVars 0
        t = newT;
        if (t < 0.001f)
            t = 0.001f;
        if (t > 0.999f)
            t = 0.999f;

        const float z0 = t * 127.f;
        const float z1 = (1.f - t) * 127.f;
        const float s0 = -1.f * (127.f - z0) / (127.f - z1);
        const float s1 = -1.f * z0 / z1;

        const float b0 = z0 * 0.5f;
        const float b1 = z1 * 0.5f;
        const float b2 = (127.f + z0) * 0.5f;
        const float b3 = (127.f + z1) * 0.5f;

        c0 = (b3 - b1 + (s0 * b0) - (s1 * b2)) / (s0 - s1);
        c1 = s1 * (b3 - b1 + (s0 * b0) - (s1 * b2));
        c1 /= (s0 - s1);
        c1 -= (s1 * b2);
        c1 += b3;
        rsq = square (c0) + square (c1);

#if dbgVars
        DBG ("t=" << t << " z0=" << z0 << " z1=" << z1);
        DBG ("s0=" << s0 << " s1=" << s1);
        DBG ("b0=" << b0 << " b1=" << b1 << " b2=" << b2 << " b3=" << b3);
        DBG ("c0=" << c0 << " c1=" << c1);
        DBG ("r^2=" << rsq << " r=" << sqrtf (rsq));
        DBG (newLine);
#endif
    }
};

} // namespace element
