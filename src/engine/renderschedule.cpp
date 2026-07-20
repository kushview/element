// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <unordered_map>
#include <vector>

#include <element/processor.hpp>
#include "engine/graphnode.hpp"
#include "engine/graphbuilder.hpp"
#include "engine/renderschedule.hpp"

namespace element {

RenderSchedule::RenderSchedule() = default;
RenderSchedule::~RenderSchedule() = default;

std::unique_ptr<RenderSchedule> RenderSchedule::build (GraphNode& graph,
                                                       const juce::Array<void*>& orderedNodes,
                                                       int maxBlockSize)
{
    auto schedule = std::make_unique<RenderSchedule>();

    // 1. Build the parallel (non-reused) op list and size the buffer pools.
    juce::Array<void*> ops;
    GraphBuilder builder (graph, orderedNodes, ops, /*parallel*/ true);
    schedule->totalLatency = builder.getTotalLatencySamples();

    const int numAudioBuffers = builder.buffersNeeded (PortType::Audio);
    const int numMidiBuffers = builder.buffersNeeded (PortType::Midi);

    const int width = juce::jmax (maxBlockSize, 4096);
    schedule->audioBuffers.setSize (juce::jmax (1, numAudioBuffers), width);
    schedule->audioBuffers.clear();
    for (int i = 0; i < juce::jmax (1, numMidiBuffers); ++i)
        schedule->midiBuffers.add (new juce::MidiBuffer());

    for (auto* p : ops)
        schedule->ownedOps.add (static_cast<GraphOp*> (p));

    const auto& nodeOpEnds = builder.getNodeOpEnds();
    jassert (nodeOpEnds.size() == orderedNodes.size());

    // 2. Partition ops into per-node tasks (a node with no ops is not a task).
    std::unordered_map<uint32, int> taskForNode; // nodeId -> task id
    std::unordered_map<uint32, int> orderPos; // nodeId -> position in orderedNodes

    int prevEnd = 0;
    for (int i = 0; i < orderedNodes.size(); ++i)
    {
        auto* node = static_cast<Processor*> (orderedNodes.getUnchecked (i));
        orderPos[node->nodeId] = i;

        const int end = nodeOpEnds.getUnchecked (i);
        const int count = end - prevEnd;
        if (count > 0)
        {
            // The last op in the node's range is its process kernel; the rest are
            // the node's input-gather prelude.
            auto* task = new RenderTask();
            task->kernel = schedule->ownedOps.getUnchecked (end - 1);
            task->preludeStart = schedule->preludeOps.size();
            task->numPrelude = count - 1;
            // IO nodes write shared graph state (currentAudioOutputBuffer,
            // currentMidi*Buffer), so they must not run concurrently with each
            // other: pin them to the calling audio thread.
            task->audioThreadOnly = node->isAudioIONode() || node->isMidiIONode();
            for (int k = prevEnd; k < end - 1; ++k)
                schedule->preludeOps.add (schedule->ownedOps.getUnchecked (k));

            const int taskId = schedule->tasks.size();
            schedule->tasks.add (task);
            taskForNode[node->nodeId] = taskId;
        }
        prevEnd = end;
    }

    schedule->numTasks = schedule->tasks.size();
    const int numTasks = schedule->numTasks;

    // 3. Derive the forward-edge dependency DAG from the graph's connections.
    std::vector<std::vector<int>> succ (numTasks);
    std::vector<std::vector<int>> preds (numTasks);

    auto contains = [] (const std::vector<int>& v, int x) {
        for (int e : v)
            if (e == x)
                return true;
        return false;
    };

    for (int i = graph.getNumConnections(); --i >= 0;)
    {
        const auto* c = graph.getConnection (i);

        auto st = taskForNode.find (c->sourceNode);
        auto dt = taskForNode.find (c->destNode);
        if (st == taskForNode.end() || dt == taskForNode.end())
            continue;
        if (c->sourceNode == c->destNode)
            continue;

        // Only forward edges are real dependencies; back-edges are feedback loops
        // whose consumer reads the producer's previous-block output.
        if (orderPos[c->sourceNode] >= orderPos[c->destNode])
            continue;

        const int s = st->second;
        const int d = dt->second;
        if (! contains (succ[s], d))
            succ[s].push_back (d);
        if (! contains (preds[d], s))
            preds[d].push_back (s);
    }

    // 4. Fill predecessor counts, CSR successor lists, and the initial-ready set.
    for (int t = 0; t < numTasks; ++t)
    {
        auto* task = schedule->tasks.getUnchecked (t);
        task->predecessorCount = (int) preds[t].size();
        task->pendingPreds.store ((int) preds[t].size(), std::memory_order_relaxed);
        task->firstSuccessor = schedule->successors.size();
        task->numSuccessors = (int) succ[t].size();
        for (int d : succ[t])
            schedule->successors.add (d);
        if (task->predecessorCount == 0)
            schedule->initialReady.add (t);
    }

    return schedule;
}

void RenderSchedule::renderOnThisThread (int numSamples)
{
    float* const* sharedAudio = audioBuffers.getArrayOfWritePointers();
    for (int t = 0; t < numTasks; ++t)
    {
        auto* task = tasks.getUnchecked (t);
        for (int p = 0; p < task->numPrelude; ++p)
            preludeOps.getUnchecked (task->preludeStart + p)
                ->perform (sharedAudio, midiBuffers, numSamples);
        task->kernel->perform (sharedAudio, midiBuffers, numSamples);
    }
}

std::unique_ptr<RenderSchedule> RenderSchedule::merge (juce::OwnedArray<RenderSchedule>& parts)
{
    auto merged = std::make_unique<RenderSchedule>();

    int totalAudio = 0, totalMidi = 0, totalTasks = 0, totalOps = 0, totalPrelude = 0, totalSucc = 0;
    int width = 1;
    for (auto* part : parts)
    {
        totalAudio += part->audioBuffers.getNumChannels();
        totalMidi += part->midiBuffers.size();
        totalTasks += part->numTasks;
        totalOps += part->ownedOps.size();
        totalPrelude += part->preludeOps.size();
        totalSucc += part->successors.size();
        width = juce::jmax (width, part->audioBuffers.getNumSamples());
        merged->totalLatency = juce::jmax (merged->totalLatency, part->totalLatency);
    }

    merged->ownedOps.ensureStorageAllocated (totalOps);
    merged->tasks.ensureStorageAllocated (totalTasks);
    merged->preludeOps.ensureStorageAllocated (totalPrelude);
    merged->successors.ensureStorageAllocated (totalSucc);

    int audioBase = 0, midiBase = 0, taskBase = 0, preludeBase = 0, successorBase = 0;

    for (auto* part : parts)
    {
        for (int i = 0; i < part->ownedOps.size(); ++i)
        {
            auto* op = part->ownedOps.getUnchecked (i);
            op->rebase (audioBase, midiBase);
            merged->ownedOps.add (op);
        }
        part->ownedOps.clearQuick (false);

        for (int i = 0; i < part->preludeOps.size(); ++i)
            merged->preludeOps.add (part->preludeOps.getUnchecked (i));

        for (int i = 0; i < part->tasks.size(); ++i)
        {
            auto* task = part->tasks.getUnchecked (i);
            task->preludeStart += preludeBase;
            task->firstSuccessor += successorBase;
            merged->tasks.add (task);
        }
        part->tasks.clearQuick (false);

        for (int i = 0; i < part->successors.size(); ++i)
            merged->successors.add (part->successors.getUnchecked (i) + taskBase);

        for (int i = 0; i < part->initialReady.size(); ++i)
            merged->initialReady.add (part->initialReady.getUnchecked (i) + taskBase);

        audioBase += part->audioBuffers.getNumChannels();
        midiBase += part->midiBuffers.size();
        taskBase += part->numTasks;
        preludeBase = merged->preludeOps.size();
        successorBase = merged->successors.size();
    }

    merged->numTasks = merged->tasks.size();

    // The pools are per-block scratch: allocate fresh and cleared, no data to carry over.
    merged->audioBuffers.setSize (juce::jmax (1, totalAudio), width);
    merged->audioBuffers.clear();
    for (int i = 0; i < juce::jmax (1, totalMidi); ++i)
        merged->midiBuffers.add (new juce::MidiBuffer());

    return merged;
}

} // namespace element
