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
            // audio thread's setting, so enable it for the duration of the drain.
            {
                const juce::ScopedNoDenormals noDenormals;
                pool.drain (false);
            }
            pool.workersActive.fetch_sub (1, std::memory_order_release);
        }
    }

    juce::WaitableEvent wake;
    RenderPool& pool;
};

RenderPool::RenderPool (int numThreads_, double sampleRate, int blockSize)
    : numThreads (juce::jmax (1, numThreads_))
{
    juce::Thread::RealtimeOptions options;
    options = options.withApproximateAudioProcessingTime (juce::jmax (1, blockSize),
                                                          sampleRate > 0.0 ? sampleRate : 44100.0);

    for (int i = 1; i < numThreads; ++i)
    {
        auto* w = new Worker (*this, i);
        workers.add (w);
        w->startRealtimeThread (options);
    }
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

void RenderPool::pushTo (std::atomic<int>& head, int taskId) noexcept
{
    auto& tasks = current->tasks;
    int old = head.load (std::memory_order_relaxed);
    do
    {
        tasks.getUnchecked (taskId)->nextReady.store (old, std::memory_order_relaxed);
    } while (! head.compare_exchange_weak (old, taskId, std::memory_order_release, std::memory_order_relaxed));
}

int RenderPool::popFrom (std::atomic<int>& head) noexcept
{
    auto& tasks = current->tasks;
    int old = head.load (std::memory_order_acquire);
    while (old >= 0)
    {
        const int next = tasks.getUnchecked (old)->nextReady.load (std::memory_order_relaxed);
        if (head.compare_exchange_weak (old, next, std::memory_order_acquire, std::memory_order_relaxed))
            return old;
    }
    return -1;
}

void RenderPool::pushReady (int taskId) noexcept
{
    if (current->tasks.getUnchecked (taskId)->audioThreadOnly)
        pushTo (audioReadyHead, taskId);
    else
        pushTo (readyHead, taskId);
}

void RenderPool::drain (bool isAudioThread)
{
    ParallelSchedule& s = *current;
    const int numSamples = currentNumSamples;

    for (;;)
    {
        // The calling audio thread services its exclusive queue first; workers
        // only ever touch the shared queue.
        int t = isAudioThread ? popFrom (audioReadyHead) : -1;
        if (t < 0)
            t = popFrom (readyHead);

        if (t < 0)
        {
            if (s.completionCount.load (std::memory_order_acquire) == s.numTasks)
                return;
            cpuPause();
            continue;
        }

        RenderTask* const task = s.tasks.getUnchecked (t);
        for (int p = 0; p < task->numPrelude; ++p)
            s.preludeOps.getUnchecked (task->preludeStart + p)
                ->perform (currentAudio, s.midiBuffers, numSamples);
        task->kernel->perform (currentAudio, s.midiBuffers, numSamples);

        // Release each newly-ready successor onto the appropriate stack.
        for (int k = 0; k < task->numSuccessors; ++k)
        {
            const int succ = s.successors.getUnchecked (task->firstSuccessor + k);
            if (s.tasks.getUnchecked (succ)->pendingPreds.fetch_sub (1, std::memory_order_acq_rel) == 1)
                pushReady (succ);
        }

        s.completionCount.fetch_add (1, std::memory_order_acq_rel);
    }
}

void RenderPool::render (ParallelSchedule& schedule, int numSamples)
{
    // Per-block reset (single-threaded here, before any worker is woken).
    for (int t = 0; t < schedule.numTasks; ++t)
    {
        auto* task = schedule.tasks.getUnchecked (t);
        task->pendingPreds.store (task->predecessorCount, std::memory_order_relaxed);
    }
    schedule.completionCount.store (0, std::memory_order_relaxed);
    readyHead.store (-1, std::memory_order_relaxed);
    audioReadyHead.store (-1, std::memory_order_relaxed);

    current = &schedule;
    // Resolve channel pointers once here (single-threaded) so worker ops never
    // touch the shared AudioBuffer's bookkeeping concurrently.
    currentAudio = schedule.audioBuffers.getArrayOfWritePointers();
    currentNumSamples = numSamples;

    for (int i = 0; i < schedule.initialReady.size(); ++i)
        pushReady (schedule.initialReady.getUnchecked (i));

    // Wake the workers, then join the drain ourselves as the audio thread.
    workersActive.store (numThreads - 1, std::memory_order_release);
    for (auto* w : workers)
        w->wake.signal();

    drain (true);

    // Fork-join barrier: no worker touches pool/schedule state past this point,
    // so the next block's reset cannot race a straggling worker.
    while (workersActive.load (std::memory_order_acquire) != 0)
        cpuPause();

    current = nullptr;
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
