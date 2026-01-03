// Copyright 2014-2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI ((double) 3.14159265358979323846)
#endif
#ifndef M_SQRT2
#define M_SQRT2 ((double) 1.41421356237309504880)
#endif

namespace element {

/** A Delay-Locked Loop for MIDI clock synchronization.
 *
 * This implementation uses an adaptive bandwidth approach for faster initial
 * convergence while maintaining stability once locked. The algorithm is based
 * on a second-order loop filter design.
 */
class DelayLockedLoop
{
public:
    /** Create a new DLL */
    inline DelayLockedLoop()
    {
        reset (0.0, 1.0 / 24.0, 1.0);
    }

    /** Reset the DLL with an initial timestamp, expected period and update frequency.
     * @param now Initial timestamp
     * @param period Expected period between updates (in seconds)
     * @param freq Update frequency for bandwidth calculation
     */
    inline void reset (double now, double period, double freq)
    {
        expectedPeriod = period;
        frequency = freq;

        // Initialize timing state
        e2 = period;
        t0 = now;
        t1 = t0 + e2;

        // Reset adaptive state
        updateCount = 0;
        locked = false;

        // Start with higher bandwidth for faster initial convergence
        updateCoefficients (initialBandwidth);
    }

    /** Update the DLL with the next timestamp.
     * @param time The timestamp of the incoming event
     */
    inline void update (double time)
    {
        const double e = time - t1;

        // Update timing estimates
        t0 = t1;
        t1 += b * e + e2;
        e2 += c * e;

        // Adaptive bandwidth: transition from initial to locked bandwidth
        ++updateCount;
        if (! locked && updateCount >= lockThreshold)
        {
            locked = true;
            updateCoefficients (lockedBandwidth);
        }
    }

    /** Set the dll's bandwidth parameters.
     * @param newBandwidth Loop bandwidth
     * @param newFrequency Update frequency for normalization
     */
    inline void setParams (double newBandwidth, double newFrequency)
    {
        frequency = newFrequency;
        lockedBandwidth = newBandwidth;
        if (locked)
            updateCoefficients (newBandwidth);
    }

    /** Return the filtered period estimate (t1 - t0) in seconds */
    inline double timeDiff() const
    {
        return (t1 - t0);
    }

    /** Check if the DLL has achieved lock */
    inline bool isLocked() const
    {
        return locked;
    }

    /** Get the current estimated period */
    inline double getPeriod() const
    {
        return e2;
    }

private:
    // Timing state
    double e2 = 0;           ///< Estimated period
    double t0 = 0;           ///< Previous filtered timestamp
    double t1 = 0;           ///< Current filtered timestamp

    // Parameters
    double expectedPeriod = 1.0 / 24.0;
    double frequency = 1.0;

    // Loop filter coefficients
    double b = 0;
    double c = 0;

    // Adaptive bandwidth settings
    // Higher initial bandwidth = faster convergence but more jitter
    // Lower locked bandwidth = more stable but slower to track changes
    static constexpr double initialBandwidth = 0.5;  // Fast convergence
    static constexpr double lockedBandwidth = 0.1;   // Stable tracking
    static constexpr int lockThreshold = 24;         // Lock after ~1 beat

    int updateCount = 0;
    bool locked = false;

    /** Update loop filter coefficients based on bandwidth */
    inline void updateCoefficients (double bandwidth)
    {
        // Second-order loop filter design
        // omega is the normalized angular frequency
        const double omega = 2.0 * M_PI * bandwidth / std::max (frequency, 1.0);
        b = M_SQRT2 * omega;
        c = omega * omega;
    }
};

} // namespace element
