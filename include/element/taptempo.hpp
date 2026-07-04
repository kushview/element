// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <optional>

namespace element {

/** Computes a tempo (BPM) from a series of taps.

    Pure and hardware-free: feed it monotonic millisecond timestamps via tap()
    and it returns the averaged tempo once at least two taps have accumulated.
    Drives both the UI TAP button and MIDI-mapped tap tempo, so the logic lives
    in one testable place.

    The average is taken from the first tap of the current run: a run resets when
    the gap since the previous tap exceeds getResetInterval(), or via reset().
*/
class TapTempo {
public:
    TapTempo() = default;

    /** Clamp range applied to computed tempos, in BPM. Mirrors the manual
        tempo entry range so tapping cannot produce an out-of-range tempo. */
    void setRange (double minBpm, double maxBpm) noexcept
    {
        min = minBpm;
        max = maxBpm;
    }

    /** Gap (ms) after which the next tap starts a fresh run. */
    void setResetInterval (double ms) noexcept { resetInterval = ms; }
    double getResetInterval() const noexcept { return resetInterval; }

    /** Discard any accumulated taps. */
    void reset() noexcept { taps = 0; }

    /** Register a tap at the given millisecond timestamp (e.g. from
        juce::Time::getMillisecondCounterHiRes()).

        @returns the newly computed tempo in BPM, or std::nullopt for the first
                 tap of a run (nothing to average yet).
    */
    std::optional<double> tap (double timeMs) noexcept
    {
        if (taps == 0 || (timeMs - lastTapMs) > resetInterval) {
            firstTapMs = lastTapMs = timeMs;
            taps = 1;
            return {};
        }

        lastTapMs = timeMs;
        const double elapsed = timeMs - firstTapMs;
        const int beats = taps; // intervals counted so far
        ++taps;

        if (elapsed <= 0.0)
            return {};

        const double bpm = (static_cast<double> (beats) / elapsed) * 60000.0;
        return std::clamp (bpm, min, max);
    }

private:
    double firstTapMs = 0.0;
    double lastTapMs = 0.0;
    int taps = 0;
    double min = 20.0;
    double max = 999.0;
    double resetInterval = 2000.0;
};

} // namespace element
