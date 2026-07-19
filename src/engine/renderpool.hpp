// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>

#include <element/juce/audio_basics.hpp>
#include <element/juce/core.hpp>

namespace element {

struct ParallelSchedule;

/** A pool of real-time worker threads that renders a ParallelSchedule across
    multiple cores.

    The audio (calling) thread participates as one worker, so a pool of N threads
    uses N execution lanes with N-1 dedicated threads. Ready tasks are handed out
    through a lock-free stack; each block is a fork-join: the caller wakes the
    workers, drains alongside them, then waits for all workers to finish before
    returning. The hot path performs no allocation and takes no locks.
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

    /** Total number of execution lanes, including the calling thread. */
    int getNumThreads() const noexcept { return numThreads; }

    /** Renders the schedule to completion. Called on the audio thread; blocks
        until every task has finished. */
    void render (ParallelSchedule& schedule, int numSamples);

    /** Sets the OS audio workgroup the worker threads should join so they are
        scheduled together with the device's audio thread (macOS). Safe to call
        with a disengaged workgroup (workers then run as plain realtime threads).
        Called on the message thread; workers pick up the change on the next block. */
    void setWorkgroup (juce::AudioWorkgroup newWorkgroup);

private:
    class Worker;

    void drain (bool isAudioThread);
    void pushReady (int taskId) noexcept;
    void pushTo (std::atomic<int>& head, int taskId) noexcept;
    int popFrom (std::atomic<int>& head) noexcept;

    const int numThreads;
    juce::OwnedArray<Worker> workers;

    // Tasks any lane may run. Separately, audioThreadOnly tasks go on their own
    // stack that only the calling audio thread drains.
    std::atomic<int> readyHead { -1 };
    std::atomic<int> audioReadyHead { -1 };
    std::atomic<int> workersActive { 0 };

    juce::CriticalSection workgroupLock;
    juce::AudioWorkgroup workgroup;
    std::atomic<uint32_t> workgroupGeneration { 0 };

    ParallelSchedule* current = nullptr;
    float* const* currentAudio = nullptr;
    int currentNumSamples = 0;

    JUCE_DECLARE_NON_COPYABLE (RenderPool)
};

} // namespace element
