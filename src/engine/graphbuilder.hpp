// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include "ElementApp.h"

namespace element {

class AtomBuffer;
class GraphNode;
class Processor;

class GraphOp
{
public:
    GraphOp() {}
    virtual ~GraphOp() {}

    virtual std::string traceStep() const noexcept { return {}; }

    virtual void perform (juce::AudioSampleBuffer& sharedBufferChans,
                          const juce::OwnedArray<MidiBuffer>& sharedMidiBuffers,
                          const juce::OwnedArray<AtomBuffer>& sharedAtomBuffers,
                          const int numSamples) = 0;

private:
    JUCE_LEAK_DETECTOR (GraphOp)
};

/** Used to calculate the correct sequence of rendering ops needed, based on
    the best re-use of shared buffers at each stage. */
class GraphBuilder
{
public:
    GraphBuilder (GraphNode& graph_,
                  const Array<void*>& orderedNodes_,
                  Array<void*>& renderingOps);

    int buffersNeeded (PortType type);
    int getTotalLatencySamples() const { return totalLatency; }

private:
    //==============================================================================
    GraphNode& graph;
    const Array<void*>& orderedNodes;
    Array<uint32> allNodes[PortType::Unknown];
    Array<uint32> allPorts[PortType::Unknown];
    const uint32_t midi_MidiEvent;

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
    bool isBufferNeededLater2 (int stepIndexToSearchFrom, uint32 inputChannelOfIndexToIgnore, const uint32 sourceNode, const uint32 outputPortIndex) const;

    void markBufferAsContaining (int bufferNum, PortType type, uint32 nodeId, uint32 portIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphBuilder)
};

} // namespace element
