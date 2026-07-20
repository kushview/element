// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"

namespace element {

class GraphNode;
class Processor;

class GraphOp
{
public:
    GraphOp() {}
    virtual ~GraphOp() {}

    /** Performs the op against a shared, pre-resolved set of audio channel
        pointers and MIDI buffers.

        The audio channels are passed as a raw pointer array (resolved once per
        block by the caller) rather than as a juce::AudioBuffer, so that ops
        running concurrently on disjoint channels never touch shared AudioBuffer
        bookkeeping (its isClear flag). Every channel index has a single writer,
        so this is data-race free across parallel tasks.
    */
    virtual void perform (float* const* sharedAudio,
                          const OwnedArray<MidiBuffer>& sharedMidiBuffers,
                          const int numSamples) = 0;

    /** Offsets every audio and MIDI buffer index stored by this op.

        Used when concatenating per-graph schedules into one merged schedule
        whose buffer pools are laid end to end: each graph's ops are shifted by
        that graph's base offset into the merged pools. Pure virtual so a new op
        type cannot forget to implement it (a missed index would silently alias
        another graph's buffers).

        @param audioOffset  added to every audio channel/buffer index
        @param midiOffset   added to every MIDI buffer index
    */
    virtual void rebase (int audioOffset, int midiOffset) = 0;

private:
    JUCE_LEAK_DETECTOR (GraphOp)
};

/** Used to calculate the correct sequence of rendering ops needed, based on
    the best re-use of shared buffers at each stage. */
class GraphBuilder
{
public:
    /** Builds the rendering op sequence for a graph.

        @param parallel  When true, buffers are never reused and every node
                         processes on private buffers (see the always-copy /
                         no-reuse rules). This produces a sequence that is safe
                         to execute concurrently across independent nodes. When
                         false, the original single-threaded, buffer-reusing
                         behaviour is used.
    */
    GraphBuilder (GraphNode& graph_,
                  const Array<void*>& orderedNodes_,
                  Array<void*>& renderingOps,
                  bool parallel = false);

    int buffersNeeded (PortType type);
    int getTotalLatencySamples() const { return totalLatency; }

    /** Cumulative count of rendering ops after each ordered node was processed.
        Entry i is the number of ops in the rendering array once node i has been
        built, so node i owns ops in the half-open range
        [nodeOpEnds[i-1], nodeOpEnds[i]). Used to partition the flat op list into
        per-node tasks for parallel scheduling. */
    const Array<int>& getNodeOpEnds() const noexcept { return nodeOpEnds; }

private:
    //==============================================================================
    GraphNode& graph;
    const Array<void*>& orderedNodes;
    const bool parallel;
    Array<int> nodeOpEnds;
    Array<uint32> allNodes[PortType::Unknown];
    Array<uint32> allPorts[PortType::Unknown];

    enum
    {
        freeNodeID = 0xffffffff,
        zeroNodeID = 0xfffffffe,
        anonymousNodeID = 0xfffffffd
    };

    static bool isNodeBusy (uint32 nodeID) noexcept { return nodeID != freeNodeID && nodeID != zeroNodeID; }

    Array<uint32> nodeDelayIDs;
    Array<int> nodeDelays;
    int totalLatency;

    int getNodeDelay (const uint32 nodeID) const;
    void setNodeDelay (const uint32 nodeID, const int latency);

    int getInputLatency (const uint32 nodeID) const;

    void createRenderingOpsForNode (Processor* const node, Array<void*>& renderingOps, const int ourRenderingIndex);

    int getFreeBuffer (PortType type);
    int getReadOnlyEmptyBuffer() const noexcept;
    int getBufferContaining (const PortType type, const uint32 nodeId, const uint32 outputPort) noexcept;
    void markUnusedBuffersFree (const int stepIndex);
    bool isBufferNeededLater (int stepIndexToSearchFrom, uint32 inputChannelOfIndexToIgnore, const uint32 sourceNode, const uint32 outputPortIndex) const;
    void markBufferAsContaining (int bufferNum, PortType type, uint32 nodeId, uint32 portIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphBuilder)
};

} // namespace element
