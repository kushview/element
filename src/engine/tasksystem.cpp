// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasksystem.hpp"

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#include <emmintrin.h>
#endif

namespace element {

namespace {

constexpr int maxWorkerThreads = 64;
constexpr int spinsBeforeBlocking = 4000;

/** Hints to the CPU that the calling thread is in a spin-wait loop. */
inline void cpuRelax() noexcept
{
#if defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ volatile ("yield");
#elif defined(_M_ARM64) || defined(_M_ARM)
    __yield();
#endif
}

} // namespace

//=============================================================================
class TaskManager::WorkerThread : public juce::Thread
{
public:
    WorkerThread (TaskManager& ownerManager, int threadIndex)
        : juce::Thread ("ElementWorker_" + juce::String (threadIndex)),
          manager (ownerManager)
    {
    }

    ~WorkerThread() override
    {
        stopThread (1000);
    }

    /** Starts the thread at the best priority the options and system allow. */
    void start (const Options& opts)
    {
        if (opts.allowRealtime)
        {
            const auto realtime = juce::Thread::RealtimeOptions {}
                                      .withApproximateAudioProcessingTime (opts.blockSize, opts.sampleRate);
            if (startRealtimeThread (realtime))
                return;
        }

        startThread (juce::Thread::Priority::highest);
    }

    void run() override
    {
        // must be created and destroyed on this thread.
        juce::WorkgroupToken token;
        int joinedGeneration = -1;

        while (! threadShouldExit())
        {
            manager.workSemaphore.acquire();

            if (threadShouldExit())
                return;

            manager.wakeups.fetch_sub (1, std::memory_order_acq_rel);

            const int generation = manager.workgroupGeneration.load (std::memory_order_acquire);
            if (generation != joinedGeneration)
            {
                manager.getWorkgroup().join (token);
                joinedGeneration = generation;
            }

            manager.runClaimedJobs();
        }
    }

private:
    TaskManager& manager;
};

//=============================================================================
TaskManager::TaskManager (const Options& opts, Job jobToRun)
    : options (opts),
      job (std::move (jobToRun))
{
    jassert (job != nullptr);
    startWorkers();
}

TaskManager::~TaskManager()
{
    stopWorkers();
}

void TaskManager::configure (const Options& newOptions)
{
    if (options == newOptions)
        return;

    stopWorkers();
    options = newOptions;
    startWorkers();
}

void TaskManager::setWorkgroup (const juce::AudioWorkgroup& newWorkgroup)
{
    {
        const juce::SpinLock::ScopedLockType sl (workgroupLock);
        if (workgroup == newWorkgroup)
            return;
        workgroup = newWorkgroup;
    }

    workgroupGeneration.fetch_add (1, std::memory_order_acq_rel);
}

juce::AudioWorkgroup TaskManager::getWorkgroup() const
{
    const juce::SpinLock::ScopedLockType sl (workgroupLock);
    return workgroup;
}

void TaskManager::run (int numJobs)
{
    if (numJobs <= 0)
        return;

    doneEvent.reset();
    remaining.store (numJobs, std::memory_order_release);
    // publishing the pending count is what makes the jobs claimable.
    pending.store (numJobs, std::memory_order_release);

    // Wake only as many workers as can be useful, and never stack up more
    // permits than there are workers if they are slow to get scheduled.
    const int numWorkers = getNumThreads();
    const int outstanding = wakeups.load (std::memory_order_acquire);
    const int toWake = std::min (numJobs - 1, numWorkers - outstanding);
    if (toWake > 0)
    {
        wakeups.fetch_add (toWake, std::memory_order_acq_rel);
        workSemaphore.release (toWake);
    }

    runClaimedJobs();

    for (int i = 0; i < spinsBeforeBlocking; ++i)
    {
        if (remaining.load (std::memory_order_acquire) == 0)
            return;
        cpuRelax();
    }

    // A worker finishing the previous run can signal late, so the event
    // alone is not proof that this run has completed.
    while (remaining.load (std::memory_order_acquire) != 0)
        doneEvent.wait (-1);
}

int TaskManager::claim() noexcept
{
    int n = pending.load (std::memory_order_acquire);
    while (n > 0 && ! pending.compare_exchange_weak (n, n - 1, std::memory_order_acq_rel, std::memory_order_acquire))
    {
    }
    return n - 1;
}

void TaskManager::runClaimedJobs()
{
    for (int index = claim(); index >= 0; index = claim())
    {
        job (index);

        if (remaining.fetch_sub (1, std::memory_order_acq_rel) == 1)
            doneEvent.signal();
    }
}

void TaskManager::startWorkers()
{
    const int targetThreads = juce::jlimit (1, maxWorkerThreads, options.numThreads);
    workers.reserve (static_cast<size_t> (targetThreads));

    for (int i = 0; i < targetThreads; ++i)
    {
        auto worker = std::make_unique<WorkerThread> (*this, i);
        worker->start (options);
        workers.push_back (std::move (worker));
    }
}

void TaskManager::stopWorkers()
{
    for (auto& worker : workers)
        worker->signalThreadShouldExit();

    // Release enough permits to wake up all threads so they hit threadShouldExit()
    workSemaphore.release (static_cast<std::ptrdiff_t> (workers.size()));
    workers.clear();

    // discard any permits the exiting workers didn't consume.
    while (workSemaphore.try_acquire())
    {
    }
    wakeups.store (0, std::memory_order_release);
}

} // namespace element
