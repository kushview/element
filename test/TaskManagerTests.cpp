// SPDX-FileCopyrightText: Copyright (C) Kushview, LLC.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <boost/test/unit_test.hpp>

#include <array>
#include <atomic>
#include <thread>

#include "engine/tasksystem.hpp"

using namespace element;

namespace {

constexpr int maxJobs = 32;

TaskManager::Options testOptions (int numThreads)
{
    TaskManager::Options options;
    options.numThreads = numThreads;
    // realtime scheduling needs privileges CI machines may not have.
    options.allowRealtime = false;
    return options;
}

/** Counts how many times each job index has been run. */
struct JobCounter
{
    std::array<std::atomic<int>, maxJobs> counts {};

    TaskManager::Job job()
    {
        return [this] (int index) { counts[(size_t) index].fetch_add (1); };
    }

    /** True if indexes below numJobs ran exactly once and the rest not at all. */
    bool ranExactlyOnce (int numJobs) const
    {
        for (int i = 0; i < maxJobs; ++i)
            if (counts[(size_t) i].load() != (i < numJobs ? 1 : 0))
                return false;
        return true;
    }

    void reset()
    {
        for (auto& c : counts)
            c.store (0);
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE (TaskManagerTests)

BOOST_AUTO_TEST_CASE (EveryJobRunsExactlyOncePerRun)
{
    JobCounter counter;
    TaskManager manager (testOptions (4), counter.job());
    BOOST_REQUIRE_EQUAL (manager.getNumThreads(), 4);

    // back to back runs of varying size exercise workers that wake late
    // from one run while the next is already being published.
    for (int iteration = 0; iteration < 5000; ++iteration)
    {
        const int numJobs = iteration % (maxJobs + 1);
        counter.reset();
        manager.run (numJobs);
        BOOST_REQUIRE_MESSAGE (counter.ranExactlyOnce (numJobs),
                               "iteration " << iteration << " with " << numJobs << " jobs");
    }
}

BOOST_AUTO_TEST_CASE (RunReturnsOnlyWhenAllJobsAreDone)
{
    std::atomic<int> finished { 0 };
    TaskManager manager (testOptions (3), [&finished] (int) {
        std::this_thread::sleep_for (std::chrono::milliseconds (2));
        finished.fetch_add (1);
    });

    for (int iteration = 0; iteration < 20; ++iteration)
    {
        finished.store (0);
        manager.run (8);
        BOOST_REQUIRE_EQUAL (finished.load(), 8);
    }
}

BOOST_AUTO_TEST_CASE (ReconfigureBetweenRuns)
{
    JobCounter counter;
    TaskManager manager (testOptions (2), counter.job());

    for (const int numThreads : { 1, 8, 3, 16, 2 })
    {
        manager.configure (testOptions (numThreads));
        BOOST_REQUIRE_EQUAL (manager.getNumThreads(), numThreads);

        for (int iteration = 0; iteration < 200; ++iteration)
        {
            counter.reset();
            manager.run (maxJobs);
            BOOST_REQUIRE (counter.ranExactlyOnce (maxJobs));
        }
    }
}

BOOST_AUTO_TEST_CASE (SetWorkgroupWhileRunning)
{
    JobCounter counter;
    TaskManager manager (testOptions (4), counter.job());

    std::atomic<bool> keepGoing { true };
    std::thread setter ([&] {
        while (keepGoing.load())
            manager.setWorkgroup ({});
    });

    for (int iteration = 0; iteration < 1000; ++iteration)
    {
        counter.reset();
        manager.run (maxJobs);
        BOOST_REQUIRE (counter.ranExactlyOnce (maxJobs));
    }

    keepGoing.store (false);
    setter.join();
}

BOOST_AUTO_TEST_CASE (StartAndStopWithoutRunning)
{
    for (int i = 0; i < 50; ++i)
    {
        TaskManager manager (testOptions (8), [] (int) {});
        BOOST_REQUIRE_EQUAL (manager.getNumThreads(), 8);
    }
}

BOOST_AUTO_TEST_SUITE_END()
