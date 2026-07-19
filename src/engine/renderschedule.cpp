// Copyright 2026 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <unordered_map>
#include <vector>

#include <element/processor.hpp>
#include "engine/graphnode.hpp"
#include "engine/graphbuilder.hpp"
#include "engine/renderschedule.hpp"

namespace element {

ParallelSchedule::ParallelSchedule() = default;
ParallelSchedule::~ParallelSchedule() = default;

std::unique_ptr<ParallelSchedule> buildParallelSchedule (GraphNode& graph,
                                                         const juce::Array<void*>& orderedNodes,
                                                         int maxBlockSize)
{
    auto schedule = std::make_unique<ParallelSchedule>();

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

} // namespace element
