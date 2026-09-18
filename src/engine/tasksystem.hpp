// SPDX-FileCopyrightText: 2026 Kushview, LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <vector>
#include <semaphore>

#include <element/juce.hpp>

namespace element {

struct MultithreadingParams
{
    bool enabled { false };
    int threadCount { 4 };

    bool operator== (const MultithreadingParams&) const = default;
};

/** Lightweight work unit passed into TaskManager. */
struct Task
{
    using Function = std::function<void()>;

    juce::String name;
    Function work;
};

/** Shared completion state for individual or batched tasks. */
struct TaskState
{
    std::atomic<int> remainingTasks { 0 };
    juce::WaitableEvent doneEvent;
};

using TaskHandle = std::shared_ptr<TaskState>;

/** Thread-safe queue and thread-pool manager that coordinates background tasks. */
class TaskManager
{
public:
    explicit TaskManager (int numWorkerThreads = 4);
    ~TaskManager();

    /** Sets the number of active worker threads in the pool. */
    void setNumThreads (int numThreads);

    /** Returns the current number of worker threads in the pool. */
    int getNumThreads() const { return static_cast<int> (workers.size()); }

    /** Enqueues a single task and returns a handle for polling or waiting. */
    TaskHandle postTask (Task task);

    /** Enqueues a single lambda function and returns a handle. */
    TaskHandle postTask (Task::Function work, const juce::String& name = juce::String());

    /** Enqueues a list of tasks sharing a single task handle. */
    TaskHandle postTasks (std::vector<Task> tasks);

    /** Enqueues a list of lambda functions sharing a single task handle. */
    TaskHandle postTasks (const std::vector<Task::Function>& workItems, const juce::String& baseName = juce::String());

    /** Polls a task handle to check if all associated work is complete. */
    static bool isDone (const TaskHandle& handle);

    /** Blocks the calling thread until all work associated with the handle is complete.
        Pass millisecondsToWait = -1 to wait indefinitely. Returns true if completed. */
    static bool wait (const TaskHandle& handle, int millisecondsToWait = -1);

private:
    struct InternalTask
    {
        Task task;
        TaskHandle handle;
    };

    class WorkerThread;

    bool popTask (InternalTask& taskOut);
    void stopWorkers();

    juce::CriticalSection lock;
    std::queue<InternalTask> taskQueue;
    std::vector<std::unique_ptr<WorkerThread>> workers;
    std::counting_semaphore<1024> workSemaphore { 0 };
};

} // namespace element
