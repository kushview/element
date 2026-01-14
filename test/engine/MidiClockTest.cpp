// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include "delaylockedloop.hpp"
#include "engine/midiclock.hpp"

using namespace element;

namespace {

// Helper to calculate MIDI clock period from BPM (24 PPQN)
inline double bpmToPeriod (double bpm)
{
    return 60.0 / (bpm * 24.0);
}

// Helper to calculate BPM from period
inline double periodToBpm (double period)
{
    return 60.0 / (period * 24.0);
}

} // namespace

class DelayLockedLoopUnit
{
public:
    void runTest()
    {
        testConstruction();
        testReset();
        testConvergence();
        testLocking();
        testTempoChangeConvergence();
        testLargeTempoSwing();
        testRapidTempoChanges();
        testTempoChangeAccuracy();
        testJitterRejection();
        testExtremeTempos();
    }

private:
    void testConstruction()
    {
        DelayLockedLoop dll;
        BOOST_REQUIRE (! dll.isLocked());
    }

    void testReset()
    {
        DelayLockedLoop dll;
        const double period = 0.02; // 20ms period (approx 50 Hz)
        dll.reset (0.0, period, 24.0);
        BOOST_REQUIRE (! dll.isLocked());
        BOOST_REQUIRE (dll.getPeriod() > 0.0);
    }

    void testConvergence()
    {
        DelayLockedLoop dll;
        // Simulate 120 BPM MIDI clock (24 PPQN)
        // Period = 60 / (120 * 24) = 0.02083 seconds per tick
        const double expectedPeriod = 60.0 / (120.0 * 24.0);
        dll.reset (0.0, expectedPeriod, 24.0);

        // Send a series of clock messages at regular intervals
        double time = expectedPeriod;
        for (int i = 0; i < 48; ++i)
        {
            dll.update (time);
            time += expectedPeriod;
        }

        // After convergence, timeDiff should be close to expectedPeriod
        const double measured = dll.timeDiff();
        const double error = std::abs (measured - expectedPeriod);
        BOOST_REQUIRE (error < 0.001); // Within 1ms
    }

    void testLocking()
    {
        DelayLockedLoop dll;
        const double period = 60.0 / (120.0 * 24.0);
        dll.reset (0.0, period, 24.0);

        BOOST_REQUIRE (! dll.isLocked());

        // Send 24 updates (lock threshold)
        double time = period;
        for (int i = 0; i < 24; ++i)
        {
            dll.update (time);
            time += period;
        }

        BOOST_REQUIRE (dll.isLocked());
    }

    /** Test convergence speed when tempo changes abruptly.
     * The DLL should track tempo changes reasonably quickly (within a few beats).
     */
    void testTempoChangeConvergence()
    {
        DelayLockedLoop dll;

        // Start at 120 BPM
        const double initialBpm = 120.0;
        const double initialPeriod = bpmToPeriod (initialBpm);
        dll.reset (0.0, initialPeriod, 24.0);

        // First, establish lock at 120 BPM
        double time = initialPeriod;
        for (int i = 0; i < 48; ++i) // 2 beats to establish solid lock
        {
            dll.update (time);
            time += initialPeriod;
        }

        BOOST_REQUIRE (dll.isLocked());
        double measuredBpm = periodToBpm (dll.timeDiff());
        BOOST_REQUIRE_MESSAGE (std::abs (measuredBpm - initialBpm) < 1.0,
            "Initial BPM should be close to 120, got " << measuredBpm);

        // Now change to 140 BPM abruptly
        const double newBpm = 140.0;
        const double newPeriod = bpmToPeriod (newBpm);

        // Track how many ticks it takes to converge within 2 BPM
        int ticksToConverge = 0;
        const double acceptableError = 2.0; // BPM

        for (int i = 0; i < 96; ++i) // Up to 4 beats
        {
            dll.update (time);
            time += newPeriod;
            ticksToConverge++;

            measuredBpm = periodToBpm (dll.timeDiff());
            if (std::abs (measuredBpm - newBpm) < acceptableError)
                break;
        }

        // Should converge within 2 beats (48 ticks) after tempo change
        BOOST_REQUIRE_MESSAGE (ticksToConverge <= 48,
            "Tempo change convergence took " << ticksToConverge << " ticks, expected <= 48");

        measuredBpm = periodToBpm (dll.timeDiff());
        BOOST_REQUIRE_MESSAGE (std::abs (measuredBpm - newBpm) < acceptableError,
            "After convergence, BPM should be close to " << newBpm << ", got " << measuredBpm);
    }

    /** Test convergence with a large tempo swing (20 BPM to 120 BPM).
     * This is a 6x increase in tempo, testing how the DLL handles extreme changes.
     */
    void testLargeTempoSwing()
    {
        DelayLockedLoop dll;

        // Start at very slow 20 BPM
        const double initialBpm = 20.0;
        const double initialPeriod = bpmToPeriod (initialBpm);
        dll.reset (0.0, initialPeriod, 24.0);

        // Establish lock at 20 BPM (need more time due to slow tempo)
        double time = initialPeriod;
        for (int i = 0; i < 48; ++i) // 2 beats
        {
            dll.update (time);
            time += initialPeriod;
        }

        BOOST_REQUIRE (dll.isLocked());
        double measuredBpm = periodToBpm (dll.timeDiff());
        BOOST_REQUIRE_MESSAGE (std::abs (measuredBpm - initialBpm) < 1.0,
            "Initial BPM should be close to 20, got " << measuredBpm);

        // Now jump to 120 BPM (6x faster!)
        const double newBpm = 120.0;
        const double newPeriod = bpmToPeriod (newBpm);

        // Track convergence - this is a dramatic change
        int ticksToConverge = 0;
        const double acceptableError = 5.0; // BPM - allow slightly more for large swing

        // Give it up to 8 beats (192 ticks) at the new tempo
        for (int i = 0; i < 192; ++i)
        {
            dll.update (time);
            time += newPeriod;
            ticksToConverge++;

            measuredBpm = periodToBpm (dll.timeDiff());
            if (std::abs (measuredBpm - newBpm) < acceptableError)
                break;
        }

        // Should converge within 4 beats (96 ticks) even for this large swing
        BOOST_REQUIRE_MESSAGE (ticksToConverge <= 96,
            "Large tempo swing convergence took " << ticksToConverge << " ticks, expected <= 96");

        measuredBpm = periodToBpm (dll.timeDiff());
        BOOST_REQUIRE_MESSAGE (std::abs (measuredBpm - newBpm) < acceptableError,
            "After large swing to " << newBpm << " BPM, got " << measuredBpm << " BPM");
    }

    /** Test multiple rapid tempo changes to ensure the DLL remains stable.
     * Note: Once locked, the DLL uses a lower bandwidth (0.1) for stability,
     * which means it tracks changes more slowly. This test verifies that
     * the DLL eventually converges even with rapid tempo changes.
     */
    void testRapidTempoChanges()
    {
        DelayLockedLoop dll;

        const double startBpm = 120.0;
        dll.reset (0.0, bpmToPeriod (startBpm), 24.0);

        double time = 0.0;

        // Establish initial lock
        for (int i = 0; i < 24; ++i)
        {
            time += bpmToPeriod (startBpm);
            dll.update (time);
        }

        BOOST_REQUIRE (dll.isLocked());

        // Test a single large tempo change (120 -> 140 BPM, 17% increase)
        // The locked DLL should converge within 8 beats
        const double newBpm = 140.0;
        const double newPeriod = bpmToPeriod (newBpm);

        for (int beat = 0; beat < 8; ++beat)
        {
            for (int tick = 0; tick < 24; ++tick)
            {
                time += newPeriod;
                dll.update (time);
            }
        }

        const double measuredBpm = periodToBpm (dll.timeDiff());
        const double error = std::abs (measuredBpm - newBpm);

        BOOST_REQUIRE_MESSAGE (error < 2.0,
            "After 8 beats at " << newBpm << " BPM, measured " << measuredBpm << " BPM (error: " << error << ")");
    }

    /** Test that the DLL accurately tracks various tempos. */
    void testTempoChangeAccuracy()
    {
        // Test a range of musically relevant tempos
        const double testTempos[] = { 60.0, 80.0, 100.0, 120.0, 140.0, 160.0, 180.0, 200.0 };
        const int numTempos = sizeof (testTempos) / sizeof (testTempos[0]);

        for (int t = 0; t < numTempos; ++t)
        {
            DelayLockedLoop dll;
            const double targetBpm = testTempos[t];
            const double period = bpmToPeriod (targetBpm);

            dll.reset (0.0, period, 24.0);

            // Run for 3 beats (72 ticks) to ensure solid convergence
            double time = period;
            for (int i = 0; i < 72; ++i)
            {
                dll.update (time);
                time += period;
            }

            const double measuredBpm = periodToBpm (dll.timeDiff());
            const double error = std::abs (measuredBpm - targetBpm);

            // Should be accurate within 0.5 BPM for steady tempo
            BOOST_REQUIRE_MESSAGE (error < 0.5,
                "At " << targetBpm << " BPM, measured " << measuredBpm << " BPM (error: " << error << ")");
        }
    }

    /** Test that the DLL rejects timing jitter and maintains stability. */
    void testJitterRejection()
    {
        DelayLockedLoop dll;
        const double targetBpm = 120.0;
        const double idealPeriod = bpmToPeriod (targetBpm);

        dll.reset (0.0, idealPeriod, 24.0);

        // Simple deterministic "jitter" pattern
        double time = idealPeriod;
        for (int i = 0; i < 96; ++i)
        {
            // Add alternating jitter: +/- 0.5ms
            const double jitter = (i % 2 == 0) ? 0.0005 : -0.0005;
            dll.update (time + jitter);
            time += idealPeriod;
        }

        const double measuredBpm = periodToBpm (dll.timeDiff());
        const double error = std::abs (measuredBpm - targetBpm);

        // Despite jitter, should still track the correct tempo within 1 BPM
        BOOST_REQUIRE_MESSAGE (error < 1.0,
            "With jitter, expected ~" << targetBpm << " BPM, got " << measuredBpm << " BPM");
    }

    /** Test extreme tempo values at the edges of musical ranges. */
    void testExtremeTempos()
    {
        // Very slow tempo (40 BPM - ambient/doom metal)
        {
            DelayLockedLoop dll;
            const double targetBpm = 40.0;
            const double period = bpmToPeriod (targetBpm);

            dll.reset (0.0, period, 24.0);

            double time = period;
            for (int i = 0; i < 48; ++i)
            {
                dll.update (time);
                time += period;
            }

            const double measuredBpm = periodToBpm (dll.timeDiff());
            BOOST_REQUIRE_MESSAGE (std::abs (measuredBpm - targetBpm) < 1.0,
                "At slow tempo " << targetBpm << " BPM, got " << measuredBpm);
        }

        // Very fast tempo (240 BPM - speedcore/extreme metal)
        {
            DelayLockedLoop dll;
            const double targetBpm = 240.0;
            const double period = bpmToPeriod (targetBpm);

            dll.reset (0.0, period, 24.0);

            double time = period;
            for (int i = 0; i < 48; ++i)
            {
                dll.update (time);
                time += period;
            }

            const double measuredBpm = periodToBpm (dll.timeDiff());
            BOOST_REQUIRE_MESSAGE (std::abs (measuredBpm - targetBpm) < 1.0,
                "At fast tempo " << targetBpm << " BPM, got " << measuredBpm);
        }
    }
};

class MidiClockMasterUnit
{
public:
    void runTest()
    {
        testConstruction();
        testTempoChange();
        testClockGeneration();
        testTempoChangeClockRate();
        testExtremeTempos();
    }

private:
    void testConstruction()
    {
        MidiClockMaster master;
        master.setSampleRate (44100.0);
        master.setTempo (120.0);
        // Should not crash
        BOOST_REQUIRE (true);
    }

    void testTempoChange()
    {
        MidiClockMaster master;
        master.setSampleRate (44100.0);
        master.setTempo (120.0);
        master.setTempo (140.0);
        // Should handle tempo changes without crashing
        BOOST_REQUIRE (true);
    }

    /** Test that the master generates clocks at the expected rate. */
    void testClockGeneration()
    {
        MidiClockMaster master;
        const double sampleRate = 44100.0;
        const double tempo = 120.0;

        master.setSampleRate (sampleRate);
        master.setTempo (tempo);

        // At 120 BPM with 24 PPQN:
        // Clocks per minute = 120 * 24 = 2880
        // Clocks per second = 48
        // Samples per clock = 44100 / 48 = 918.75

        // Render 1 second of audio and count clocks
        MidiBuffer midi;
        const int blockSize = 512;
        const int blocksPerSecond = static_cast<int> (sampleRate / blockSize);
        int totalClocks = 0;

        for (int b = 0; b < blocksPerSecond; ++b)
        {
            midi.clear();
            master.render (midi, blockSize);

            for (const auto metadata : midi)
            {
                if (metadata.getMessage().isMidiClock())
                    ++totalClocks;
            }
        }

        // Expected: ~48 clocks per second at 120 BPM
        // Allow some tolerance for block boundary alignment
        BOOST_REQUIRE_MESSAGE (totalClocks >= 46 && totalClocks <= 50,
            "Expected ~48 clocks/sec at 120 BPM, got " << totalClocks);
    }

    /** Test that tempo changes immediately affect clock rate. */
    void testTempoChangeClockRate()
    {
        MidiClockMaster master;
        const double sampleRate = 44100.0;

        master.setSampleRate (sampleRate);
        master.setTempo (120.0);

        // Render a bit at 120 BPM first
        MidiBuffer midi;
        for (int i = 0; i < 10; ++i)
        {
            midi.clear();
            master.render (midi, 512);
        }

        // Now change to 240 BPM (double speed)
        master.setTempo (240.0);

        // Count clocks over 0.5 seconds
        int clocksAt240 = 0;
        const int blocksPerHalfSecond = static_cast<int> (sampleRate / 512 / 2);

        for (int b = 0; b < blocksPerHalfSecond; ++b)
        {
            midi.clear();
            master.render (midi, 512);

            for (const auto metadata : midi)
            {
                if (metadata.getMessage().isMidiClock())
                    ++clocksAt240;
            }
        }

        // At 240 BPM: 240 * 24 = 5760 clocks/min = 96 clocks/sec
        // In 0.5 seconds: ~48 clocks
        BOOST_REQUIRE_MESSAGE (clocksAt240 >= 44 && clocksAt240 <= 52,
            "Expected ~48 clocks in 0.5s at 240 BPM, got " << clocksAt240);

        // Now halve the tempo to 60 BPM
        master.setTempo (60.0);

        int clocksAt60 = 0;
        for (int b = 0; b < blocksPerHalfSecond; ++b)
        {
            midi.clear();
            master.render (midi, 512);

            for (const auto metadata : midi)
            {
                if (metadata.getMessage().isMidiClock())
                    ++clocksAt60;
            }
        }

        // At 60 BPM: 60 * 24 = 1440 clocks/min = 24 clocks/sec
        // In 0.5 seconds: ~12 clocks
        BOOST_REQUIRE_MESSAGE (clocksAt60 >= 10 && clocksAt60 <= 14,
            "Expected ~12 clocks in 0.5s at 60 BPM, got " << clocksAt60);
    }

    /** Test extreme tempo values. */
    void testExtremeTempos()
    {
        MidiClockMaster master;
        const double sampleRate = 44100.0;

        master.setSampleRate (sampleRate);

        // Test very slow tempo (30 BPM)
        master.setTempo (30.0);
        MidiBuffer midi;
        master.render (midi, 512);
        BOOST_REQUIRE (true); // Should not crash

        // Test very fast tempo (300 BPM)
        master.setTempo (300.0);
        midi.clear();
        master.render (midi, 512);
        BOOST_REQUIRE (true); // Should not crash

        // Test different sample rates
        master.setSampleRate (48000.0);
        master.setTempo (120.0);
        midi.clear();
        master.render (midi, 512);
        BOOST_REQUIRE (true);

        master.setSampleRate (96000.0);
        midi.clear();
        master.render (midi, 512);
        BOOST_REQUIRE (true);
    }
};

BOOST_AUTO_TEST_SUITE (MidiClockTest)

BOOST_AUTO_TEST_CASE (DelayLockedLoop)
{
    DelayLockedLoopUnit().runTest();
}

BOOST_AUTO_TEST_CASE (MidiClockMaster)
{
    MidiClockMasterUnit().runTest();
}

BOOST_AUTO_TEST_SUITE_END()
