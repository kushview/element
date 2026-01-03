// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>
#include "delaylockedloop.hpp"
#include "engine/midiclock.hpp"

using namespace element;

class DelayLockedLoopUnit
{
public:
    void runTest()
    {
        testConstruction();
        testReset();
        testConvergence();
        testLocking();
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
};

class MidiClockMasterUnit
{
public:
    void runTest()
    {
        testConstruction();
        testTempoChange();
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
