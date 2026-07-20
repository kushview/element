// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <memory>

#include <element/juce/audio_basics.hpp>
#include <element/juce/core.hpp>

namespace element {

struct RenderSchedule;

/** A pool of real-time worker threads that renders RenderSchedules across
    multiple cores.

    The pool accepts multiple concurrent jobs: each render() call claims a job
    slot, publishes its schedule, and participates in the drain as one worker
    for its own job while the shared worker threads help every active job. This
    lets several engine instances (e.g. plugin instances whose processBlock
    calls run concurrently on different host threads) share one process-wide
    pool without oversubscribing the machine.

    Ready tasks are handed out through per-job lock-free stacks. Tasks flagged
    audioThreadOnly are drained only by the thread that submitted that job. The
    hot path performs no allocation and takes no locks; waits are a bounded
    spin that falls back to yielding so a stalled drain never starves the rest
    of the system (or a plugin host's own workers).

    A schedule must not be submitted concurrently with itself; each engine
    submits its own schedule from its own (single) audio thread.
*/
class RenderPool
{
public:
    /** Creates and starts the pool.
        @param numThreads  total execution lanes (>= 1); numThreads-1 real-time
                           worker threads are started.
        @param sampleRate  used to size the worker real-time scheduling budget.
        @param blockSize   used to size the worker real-time scheduling budget.
    */
    RenderPool (int numThreads, double sampleRate, int blockSize);
    ~RenderPool();

    /** Returns the process-wide shared pool, creating it on first use.

        All graphs and engine instances in the process share one pool so the
        total worker thread count stays bounded regardless of how many engines
        enable multicore rendering. The pool is sized from the CPU count. If a
        live pool exists with a different sample rate or block size, a new pool
        replaces it for subsequent acquirers; the old one is destroyed when its
        last reference is released.

        @param sampleRate  used to size the worker real-time scheduling budget.
        @param blockSize   used to size the worker real-time scheduling budget.
    */
    static std::shared_ptr<RenderPool> acquireShared (double sampleRate, int blockSize);

    /** Total number of execution lanes actually available, including the calling
        thread. May be less than requested if realtime worker threads could not be
        started (e.g. a Linux host without realtime scheduling privileges). */
    int getNumThreads() const noexcept { return numThreads; }

    /** The sample rate this pool's worker budget was sized for. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** The block size this pool's worker budget was sized for. */
    int getBlockSize() const noexcept { return blockSize; }

    /** Renders the schedule to completion as one job. Called on the submitting
        engine's audio thread; blocks until every task has finished. When all
        job slots are busy the schedule is rendered serially on the calling
        thread instead (correct, just not accelerated). */
    void render (RenderSchedule& schedule, int numSamples);

    /** Sets the OS audio workgroup the worker threads should join so they are
        scheduled together with the audio thread (macOS). Safe to call with a
        disengaged workgroup (workers then run as plain realtime threads). With
        several engines sharing the pool the last writer wins. Workers pick up
        the change on their next wake. */
    void setWorkgroup (juce::AudioWorkgroup newWorkgroup);

private:
    class Worker;

    /** One in-flight render() call. The submitting thread owns audioReadyHead;
        workers pull from readyHead only. `schedule` doubles as the publication
        flag: a non-null acquire-load means the job is initialized and joinable. */
    struct Job
    {
        std::atomic<RenderSchedule*> schedule { nullptr };
        float* const* audio = nullptr;
        int numSamples = 0;
        std::atomic<int> readyHead { -1 };
        std::atomic<int> audioReadyHead { -1 };
        /** Workers currently inside the job (pin count); the submitter waits
            for zero after unpublishing before its schedule may be freed. */
        std::atomic<int> inFlight { 0 };
        std::atomic<bool> claimed { false };
    };

    static constexpr int maxJobs = 8;

    void runTask (Job& job, RenderSchedule& s, int taskId) noexcept;
    bool helpSomeJob() noexcept;
    void pushReady (Job& job, RenderSchedule& s, int taskId) noexcept;
    static void pushTo (std::atomic<int>& head, RenderSchedule& s, int taskId) noexcept;
    static int popFrom (std::atomic<int>& head, RenderSchedule& s) noexcept;

    int numThreads { 1 };
    double sampleRate { 0.0 };
    int blockSize { 0 };
    juce::OwnedArray<Worker> workers;

    Job jobs[maxJobs];
    std::atomic<int> numActiveJobs { 0 };

    juce::CriticalSection workgroupLock;
    juce::AudioWorkgroup workgroup;
    std::atomic<uint32_t> workgroupGeneration { 0 };

    JUCE_DECLARE_NON_COPYABLE (RenderPool)
};

} // namespace element
