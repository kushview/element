// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <memory>

#include <element/juce/audio_basics.hpp>
#include <element/juce/core.hpp>

namespace element {

class GraphOp;
class GraphNode;

/** One node's unit of work in a parallel render schedule.

    A task is the node's input-gather ops (its "prelude") followed by the node's
    process kernel op. Tasks are the granularity at which the graph is scheduled
    across threads; a task always runs start-to-finish on a single thread, so a
    processor instance is never concurrent with itself.
*/
struct RenderTask
{
    RenderTask() = default;

    /** The node's process op (a ProcessBufferOp). Not owned. */
    GraphOp* kernel = nullptr;

    /** Half-open range into RenderSchedule::preludeOps for this task's gather
        ops, run before the kernel. */
    int preludeStart = 0;
    int numPrelude = 0;

    /** Half-open range into RenderSchedule::successors listing the tasks that
        depend on this one. */
    int firstSuccessor = 0;
    int numSuccessors = 0;

    /** Number of distinct forward-edge predecessor tasks. */
    int predecessorCount = 0;

    /** Intrusive link used by the lock-free ready stack (task id, or -1).
        Atomic because the Treiber stack reads it speculatively in the CAS-retry
        path; the stack head's release/acquire orders the task payload. */
    std::atomic<int> nextReady { -1 };

    /** Reserved for future user-assignable affinity; -1 means any worker. */
    int assignedWorker = -1;

    /** When true this task must run on the calling audio thread (thread-affine
        processors, IO nodes touching shared graph state, etc.). */
    bool audioThreadOnly = false;

    /** Live count of unfinished predecessors, reset to predecessorCount each
        block by the dispatcher. */
    std::atomic<int> pendingPreds { 0 };

    JUCE_DECLARE_NON_COPYABLE (RenderTask)
};

/** A baked, parallel-safe render schedule for a GraphNode.

    Owns the rendering ops and a non-reused buffer pool where every buffer index
    has exactly one writer, so independent tasks can execute concurrently without
    aliasing each other's data. Built on the message thread by RenderSchedule::build.
*/
struct RenderSchedule
{
    RenderSchedule();
    ~RenderSchedule();

    /** Builds a parallel render schedule for a graph.

        Runs on the message thread. Uses GraphBuilder in parallel mode to lay out a
        non-reused buffer pool, partitions the resulting op list into per-node tasks,
        and derives the forward-edge dependency DAG from the graph's connections.

        @param graph            the graph to schedule
        @param orderedNodes     topologically ordered Processor* pointers
        @param maxBlockSize     buffer pool width in samples
        @return the baked schedule, never null
    */
    static std::unique_ptr<RenderSchedule> build (GraphNode& graph,
                                                  const juce::Array<void*>& orderedNodes,
                                                  int maxBlockSize);

    /** Merges per-graph schedules into one schedule spanning all of them.

        Runs on the message thread. Lays the parts' buffer pools end to end and
        rebases every op's buffer indices by its graph's base offset, then
        concatenates tasks with offset ids. The parts' graphs are independent so
        no cross-graph dependency edges exist; each part's initial-ready tasks
        stay initially ready. The merged schedule takes ownership of every op
        and task; the drained parts are left empty.

        All parts must have been built with the same maxBlockSize.

        @param parts  the per-graph schedules to merge; emptied by this call
        @return the merged schedule, never null
    */
    static std::unique_ptr<RenderSchedule> merge (juce::OwnedArray<RenderSchedule>& parts);

    /** Drains every task in topological order on the calling thread.
        Used when threading isn't worthwhile or no worker pool is available. */
    void renderOnThisThread (int numSamples);

    /** Owns every rendering op (prelude ops and kernels). */
    juce::OwnedArray<GraphOp> ownedOps;

    /** Tasks in topological order; the task's index is its id. */
    juce::OwnedArray<RenderTask> tasks;

    /** Packed prelude op pointers referenced by RenderTask::preludeStart. */
    juce::Array<GraphOp*> preludeOps;

    /** CSR successor task ids referenced by RenderTask::firstSuccessor. */
    juce::Array<int> successors;

    /** Task ids that start ready (predecessorCount == 0). */
    juce::Array<int> initialReady;

    /** The shared, never-reused audio buffer pool addressed by the ops. */
    juce::AudioBuffer<float> audioBuffers;

    /** The shared MIDI buffer pool addressed by the ops. */
    juce::OwnedArray<juce::MidiBuffer> midiBuffers;

    int numTasks = 0;
    int totalLatency = 0;

    /** Per-block count of completed tasks; reset by the dispatcher each block and
        used to detect when the whole schedule has finished. */
    std::atomic<int> completionCount { 0 };
};

} // namespace element
