// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <semaphore>
#include <vector>

#include <element/juce/audio_basics.hpp>

namespace element {

struct MultithreadingParams
{
    bool enabled { false };
    int threadCount { 4 };

    bool operator== (const MultithreadingParams&) const = default;
};

/** Runs a fixed job function across a pool of worker threads.

    This is a realtime "parallel for". The job is installed once at
    construction, and each call to run() executes it for every index in
    [0, numJobs). The thread calling run() takes part in the work, so dispatch
    involves no allocation, no queue and no locks.
*/
class TaskManager
{
public:
    /** The function executed for each job index. */
    using Job = std::function<void (int jobIndex)>;

    /** Worker thread configuration. */
    struct Options
    {
        /** Number of worker threads, not counting the thread that calls run(). */
        int numThreads { 4 };

        /** If true the workers are started as realtime threads, falling back
            to the highest regular priority if the system refuses. */
        bool allowRealtime { true };

        /** The expected audio block size and sample rate. Used as a scheduling
            hint for realtime threads. */
        int blockSize { 512 };
        double sampleRate { 44100.0 };

        bool operator== (const Options&) const = default;
    };

    /** Creates the pool and starts its workers.

        @param options  the worker thread configuration
        @param job      the function to run for each job index. It will be called
                        concurrently from several threads with different indexes.
    */
    TaskManager (const Options& options, Job job);
    ~TaskManager();

    /** Applies new options, restarting the workers if anything changed.

        Not realtime safe, and must not be called while run() is in progress.
    */
    void configure (const Options& options);

    /** Returns the options currently in use. */
    const Options& getOptions() const noexcept { return options; }

    /** Returns the current number of worker threads in the pool. */
    int getNumThreads() const noexcept { return static_cast<int> (workers.size()); }

    /** Sets the audio workgroup the workers should join.

        Safe to call from the audio thread. Workers pick up the change the next
        time they wake. Has no effect on platforms without audio workgroups.
    */
    void setWorkgroup (const juce::AudioWorkgroup& workgroup);

    /** Runs the job for each index in [0, numJobs) and returns once all are done.

        Realtime safe. The calling thread runs jobs alongside the workers. Must
        only be called from one thread at a time.
    */
    void run (int numJobs);

private:
    class WorkerThread;

    int claim() noexcept;
    void runClaimedJobs();
    void startWorkers();
    void stopWorkers();
    juce::AudioWorkgroup getWorkgroup() const;

    Options options;
    const Job job;
    std::vector<std::unique_ptr<WorkerThread>> workers;
    std::counting_semaphore<1024> workSemaphore { 0 };

    // permits released to the semaphore that no worker has consumed yet.
    std::atomic<int> wakeups { 0 };

    // jobs not yet claimed, and jobs not yet finished, in the current run.
    std::atomic<int> pending { 0 };
    std::atomic<int> remaining { 0 };
    juce::WaitableEvent doneEvent;

    mutable juce::SpinLock workgroupLock;
    juce::AudioWorkgroup workgroup;
    std::atomic<int> workgroupGeneration { 0 };
};

} // namespace element
