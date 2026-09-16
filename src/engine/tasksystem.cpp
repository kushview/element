// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasksystem.hpp"

namespace element {

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

    void run() override
    {
        while (! threadShouldExit())
        {
            // Blocks until a task permit is released or shutdown occurs
            manager.workSemaphore.acquire();

            if (threadShouldExit())
                return;

            InternalTask internalTask;
            if (manager.popTask (internalTask))
            {
                if (internalTask.task.work != nullptr)
                    internalTask.task.work();

                if (internalTask.handle != nullptr)
                {
                    if (internalTask.handle->remainingTasks.fetch_sub (1, std::memory_order_acq_rel) == 1)
                    {
                        internalTask.handle->doneEvent.signal();
                    }
                }
            }
        }
    }

private:
    TaskManager& manager;
};

//=============================================================================
TaskManager::TaskManager (int numWorkerThreads)
{
    setNumThreads (numWorkerThreads);
}

TaskManager::~TaskManager()
{
    stopWorkers();
}

void TaskManager::setNumThreads (int numThreads)
{
    stopWorkers();

    const int targetThreads = std::max (1, numThreads);
    workers.reserve (targetThreads);

    for (int i = 0; i < targetThreads; ++i)
    {
        auto worker = std::make_unique<WorkerThread> (*this, i);
        worker->startThread();
        workers.push_back (std::move (worker));
    }
}

TaskHandle TaskManager::postTask (Task task)
{
    std::vector<Task> tasks;
    tasks.push_back (std::move (task));
    return postTasks (std::move (tasks));
}

TaskHandle TaskManager::postTask (Task::Function work, const juce::String& name)
{
    return postTask (Task { name, std::move (work) });
}

TaskHandle TaskManager::postTasks (std::vector<Task> tasks)
{
    auto handle = std::make_shared<TaskState>();
    handle->remainingTasks.store (static_cast<int> (tasks.size()), std::memory_order_relaxed);

    if (tasks.empty())
    {
        handle->doneEvent.signal();
        return handle;
    }

    const int numTasks = static_cast<int> (tasks.size());

    {
        juce::ScopedLock sl (lock);
        for (auto& task : tasks)
        {
            taskQueue.push ({ std::move (task), handle });
        }
    }

    // Release permits corresponding to enqueued tasks
    workSemaphore.release (numTasks);
    return handle;
}

TaskHandle TaskManager::postTasks (const std::vector<Task::Function>& workItems, const juce::String& baseName)
{
    std::vector<Task> tasks;
    tasks.reserve (workItems.size());

    int count = 0;
    for (const auto& work : workItems)
    {
        const auto name = baseName.isNotEmpty() 
                            ? baseName + "_" + juce::String (count++) 
                            : juce::String();
        tasks.push_back ({ name, work });
    }

    return postTasks (std::move (tasks));
}

bool TaskManager::isDone (const TaskHandle& handle)
{
    if (handle == nullptr)
        return true;

    return handle->remainingTasks.load (std::memory_order_acquire) <= 0;
}

bool TaskManager::wait (const TaskHandle& handle, int millisecondsToWait)
{
    if (handle == nullptr || isDone (handle))
        return true;

    return handle->doneEvent.wait (millisecondsToWait);
}

bool TaskManager::popTask (InternalTask& taskOut)
{
    juce::ScopedLock sl (lock);
    if (taskQueue.empty())
        return false;

    taskOut = std::move (taskQueue.front());
    taskQueue.pop();
    return true;
}

void TaskManager::stopWorkers()
{
    for (auto& worker : workers)
    {
        if (worker != nullptr)
            worker->signalThreadShouldExit();
    }

    // Release enough permits to wake up all threads so they hit threadShouldExit()
    workSemaphore.release (static_cast<std::ptrdiff_t> (workers.size()));

    workers.clear();
}

} // namespace element