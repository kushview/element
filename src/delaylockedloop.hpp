// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI ((float) 3.14159265358979323846)
#endif
#ifndef M_SQRT2
#define M_SQRT2 ((float) 1.41421356237309504880)
#endif

namespace element {

class DelayLockedLoop
{
public:
    /** Create a new DLL */
    inline DelayLockedLoop()
        : samplerate (44100.0),
          periodSize (1024.0),
          e2 (0),
          t0 (0),
          t1 (0),
          bandwidth (1.0f),
          frequency (0),
          omega (0),
          b (0),
          c (0)
    {
        reset (0.0, 1024.0, 44100.0);
        resetLPF();
    }

    /** Reset the DLL with a time, period and rate */
    inline void reset (double now, double period, double rate)
    {
        samplerate = rate;
        periodSize = period;

        // initialize the loop
        e2 = periodSize / samplerate;
        t0 = now;
        t1 = t0 + e2;
    }

    /** Update the DLL with the next timestamp */
    inline void update (double time)
    {
        const double e = time - t1;

        t0 = t1;
        t1 += b * e + e2;
        e2 += c * e;
    }

    /** Set the dll's parameters. Bandwidth / Frequency */
    inline void setParams (double newBandwidth, double newFrequency)
    {
        bandwidth = newBandwidth;
        frequency = newFrequency;
        resetLPF();
    }

    /**  Return the difference in filtered time (t1 - t0) */
    inline double timeDiff()
    {
        return (t1 - t0);
    }

private:
    double samplerate, periodSize;
    double e2, t0, t1;

    double bandwidth, frequency;
    double omega, b, c;

    /** @internal Reset the LPF
        Called when bandwidth and frequency changes */
    inline void resetLPF()
    {
        omega = 2.0 * M_PI * bandwidth / frequency;
        b = M_SQRT2 * omega;
        c = omega * omega;
    }
};

} // namespace element
