// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <element/processor.hpp>
#include "engine/graphbuilder.hpp"
#include "engine/renderschedule.hpp"
#include "engine/renderpool.hpp"

namespace element {

namespace {

/** Spin-loop hint so a waiting lane doesn't hammer the bus / starve its sibling
    hyper-thread. Falls back to nothing on unknown architectures. */
inline void cpuPause() noexcept
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__ ("pause");
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__ ("yield");
#endif
}

/** Bounded spin: pause-spin for a budget of iterations, then yield the thread.
    Keeps dependency stalls cheap while guaranteeing a stalled lane never
    monopolizes a core (important when nested inside a plugin host callback). */
struct BoundedSpin
{
    void wait() noexcept
    {
        cpuPause();
        if (++spins >= kSpinsBeforeYield)
        {
            spins = 0;
            juce::Thread::yield();
        }
    }

    void reset() noexcept { spins = 0; }

private:
    static constexpr int kSpinsBeforeYield = 1500;
    int spins = 0;
};

} // namespace

class RenderPool::Worker : public juce::Thread
{
public:
    Worker (RenderPool& p, int index)
        : juce::Thread ("el.render." + juce::String (index)),
          pool (p)
    {
    }

    void run() override
    {
        // The token leaves the workgroup when it is destroyed (thread exit).
        juce::WorkgroupToken token;
        uint32_t seenWorkgroupGen = (uint32_t) -1;

        while (! threadShouldExit())
        {
            wake.wait (-1);
            if (threadShouldExit())
                break;

            // Re-join the audio workgroup if it changed (rare: device start/stop).
            const uint32_t gen = pool.workgroupGeneration.load (std::memory_order_acquire);
            if (gen != seenWorkgroupGen)
            {
                seenWorkgroupGen = gen;
                token.reset();

                juce::AudioWorkgroup wg;
                {
                    const juce::ScopedLock sl (pool.workgroupLock);
                    wg = pool.workgroup;
                }
                if (wg)
                    wg.join (token);
            }

            // Denormal flushing is per-thread; worker threads don't inherit the
            // audio thread's setting, so enable it while helping.
            const juce::ScopedNoDenormals noDenormals;
            BoundedSpin idle;

            while (! threadShouldExit()
                   && pool.numActiveJobs.load (std::memory_order_acquire) > 0)
            {
                if (pool.helpSomeJob())
                    idle.reset();
                else
                    idle.wait();
            }
        }
    }

    juce::WaitableEvent wake;
    RenderPool& pool;
};

RenderPool::RenderPool (int numThreads_, double sampleRate_, int blockSize_)
    : sampleRate (sampleRate_), blockSize (blockSize_)
{
    const int requested = juce::jmax (1, numThreads_);

    juce::Thread::RealtimeOptions options;
    options = options.withApproximateAudioProcessingTime (juce::jmax (1, blockSize),
                                                          sampleRate > 0.0 ? sampleRate : 44100.0);

    for (int i = 1; i < requested; ++i)
    {
        auto w = std::make_unique<Worker> (*this, i);

        // Only keep workers that actually started. If the OS denies a realtime
        // thread (e.g. a Linux host with no RLIMIT_RTPRIO privilege), run() never
        // executes and rendering would degrade incorrectly. Stop adding lanes
        // instead; render() degrades toward the calling thread doing the work.
        if (! w->startRealtimeThread (options))
            break;

        workers.add (w.release());
    }

    numThreads = 1 + workers.size();
}

std::shared_ptr<RenderPool> RenderPool::acquireShared (double sampleRate, int blockSize)
{
    // Process-wide registry: one live pool shared by every acquirer. Guarded by
    // a mutex; only called from the message thread (prepare/rebuild), never the
    // audio thread.
    static juce::CriticalSection sharedLock;
    static std::weak_ptr<RenderPool> sharedPool;

    const juce::ScopedLock sl (sharedLock);

    if (auto existing = sharedPool.lock())
        if (existing->getSampleRate() == sampleRate && existing->getBlockSize() == blockSize)
            return existing;

    const int numThreads = juce::jlimit (1, 32, juce::SystemStats::getNumCpus());
    auto created = std::make_shared<RenderPool> (numThreads, sampleRate, blockSize);
    sharedPool = created;
    return created;
}

RenderPool::~RenderPool()
{
    for (auto* w : workers)
    {
        w->signalThreadShouldExit();
        w->wake.signal();
    }
    for (auto* w : workers)
        w->stopThread (2000);

    workers.clear();
}

void RenderPool::pushTo (std::atomic<int>& head, RenderSchedule& s, int taskId) noexcept
{
    auto& tasks = s.tasks;
    int old = head.load (std::memory_order_relaxed);
    do
    {
        tasks.getUnchecked (taskId)->nextReady.store (old, std::memory_order_relaxed);
    } while (! head.compare_exchange_weak (old, taskId, std::memory_order_release, std::memory_order_relaxed));
}

int RenderPool::popFrom (std::atomic<int>& head, RenderSchedule& s) noexcept
{
    auto& tasks = s.tasks;
    int old = head.load (std::memory_order_acquire);
    while (old >= 0)
    {
        const int next = tasks.getUnchecked (old)->nextReady.load (std::memory_order_relaxed);
        if (head.compare_exchange_weak (old, next, std::memory_order_acquire, std::memory_order_relaxed))
            return old;
    }
    return -1;
}

void RenderPool::pushReady (Job& job, RenderSchedule& s, int taskId) noexcept
{
    if (s.tasks.getUnchecked (taskId)->audioThreadOnly)
        pushTo (job.audioReadyHead, s, taskId);
    else
        pushTo (job.readyHead, s, taskId);
}

void RenderPool::runTask (Job& job, RenderSchedule& s, int taskId) noexcept
{
    RenderTask* const task = s.tasks.getUnchecked (taskId);
    for (int p = 0; p < task->numPrelude; ++p)
        s.preludeOps.getUnchecked (task->preludeStart + p)
            ->perform (job.audio, s.midiBuffers, job.numSamples);
    task->kernel->perform (job.audio, s.midiBuffers, job.numSamples);

    // Release each newly-ready successor onto the appropriate stack.
    for (int k = 0; k < task->numSuccessors; ++k)
    {
        const int succ = s.successors.getUnchecked (task->firstSuccessor + k);
        if (s.tasks.getUnchecked (succ)->pendingPreds.fetch_sub (1, std::memory_order_acq_rel) == 1)
            pushReady (job, s, succ);
    }

    s.completionCount.fetch_add (1, std::memory_order_acq_rel);
}

bool RenderPool::helpSomeJob() noexcept
{
    for (auto& job : jobs)
    {
        RenderSchedule* const s = job.schedule.load (std::memory_order_acquire);
        if (s == nullptr)
            continue;

        // Pin the job so its submitter cannot retire the schedule while this
        // thread is inside it, then re-check publication under the pin.
        job.inFlight.fetch_add (1, std::memory_order_acquire);
        if (job.schedule.load (std::memory_order_acquire) != s)
        {
            job.inFlight.fetch_sub (1, std::memory_order_release);
            continue;
        }

        const int t = popFrom (job.readyHead, *s);
        if (t >= 0)
            runTask (job, *s, t);

        job.inFlight.fetch_sub (1, std::memory_order_release);
        if (t >= 0)
            return true;
    }

    return false;
}

void RenderPool::render (RenderSchedule& schedule, int numSamples)
{
    // Claim a free job slot; if every slot is busy (more concurrent submitters
    // than slots), render serially on this thread -- correct, just unassisted.
    Job* job = nullptr;
    for (auto& candidate : jobs)
    {
        bool expected = false;
        if (candidate.claimed.compare_exchange_strong (expected, true, std::memory_order_acq_rel))
        {
            job = &candidate;
            break;
        }
    }

    if (job == nullptr || numThreads <= 1 || schedule.numTasks <= 1)
    {
        if (job != nullptr)
            job->claimed.store (false, std::memory_order_release);
        schedule.renderOnThisThread (numSamples);
        return;
    }

    // Per-block init, single-threaded: the job isn't visible to workers until
    // the schedule pointer is published below.
    for (int t = 0; t < schedule.numTasks; ++t)
    {
        auto* task = schedule.tasks.getUnchecked (t);
        task->pendingPreds.store (task->predecessorCount, std::memory_order_relaxed);
    }
    schedule.completionCount.store (0, std::memory_order_relaxed);
    job->readyHead.store (-1, std::memory_order_relaxed);
    job->audioReadyHead.store (-1, std::memory_order_relaxed);
    // Resolve channel pointers once here so worker ops never touch the shared
    // AudioBuffer's bookkeeping concurrently.
    job->audio = schedule.audioBuffers.getArrayOfWritePointers();
    job->numSamples = numSamples;

    for (int i = 0; i < schedule.initialReady.size(); ++i)
        pushReady (*job, schedule, schedule.initialReady.getUnchecked (i));

    job->schedule.store (&schedule, std::memory_order_release);
    numActiveJobs.fetch_add (1, std::memory_order_release);
    for (auto* w : workers)
        w->wake.signal();

    // Drain this job as one lane: this thread exclusively services the
    // audio-thread-only stack, and shares the rest with the workers.
    BoundedSpin idle;
    for (;;)
    {
        int t = popFrom (job->audioReadyHead, schedule);
        if (t < 0)
            t = popFrom (job->readyHead, schedule);

        if (t < 0)
        {
            if (schedule.completionCount.load (std::memory_order_acquire) == schedule.numTasks)
                break;
            idle.wait();
            continue;
        }

        idle.reset();
        runTask (*job, schedule, t);
    }

    // Retire: unpublish first so no new worker can join, then wait for any
    // worker still pinned inside the job. After this the schedule may be freed.
    job->schedule.store (nullptr, std::memory_order_release);
    idle.reset();
    while (job->inFlight.load (std::memory_order_acquire) != 0)
        idle.wait();

    numActiveJobs.fetch_sub (1, std::memory_order_release);
    job->claimed.store (false, std::memory_order_release);
}

void RenderPool::setWorkgroup (juce::AudioWorkgroup newWorkgroup)
{
    {
        const juce::ScopedLock sl (workgroupLock);
        workgroup = std::move (newWorkgroup);
    }
    workgroupGeneration.fetch_add (1, std::memory_order_release);
}

} // namespace element
