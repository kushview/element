/*
   This file is part of Element
   Copyright (c) 2014-2018 Kushview, LLC.  All rights reserved.
 */

#include "engine/nodes/AudioProcessorNode.h"
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "engine/MidiPipe.h"
#include "engine/MidiTranspose.h"
#include "engine/SubGraphProcessor.h"
#include "session/Node.h"

namespace Element {

const int GraphProcessor::midiChannelIndex = 0x1000;

namespace GraphRender
{

class Task
{
public:
    Task() { }
    virtual ~Task()  { }

    virtual void perform (AudioSampleBuffer& sharedBufferChans,
                          const OwnedArray <MidiBuffer>& sharedMidiBuffers,
                          const int numSamples) = 0;

    JUCE_LEAK_DETECTOR (Task);
};

class ClearChannelOp : public Task
{
public:
    ClearChannelOp (const int channelNum_)
        : channelNum (channelNum_)
    { }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray <MidiBuffer>&, const int numSamples)
    {
        sharedBufferChans.clear (channelNum, 0, numSamples);
    }

private:
    const int channelNum;

    JUCE_DECLARE_NON_COPYABLE (ClearChannelOp)
};


class CopyChannelOp : public Task
{
public:
    CopyChannelOp (const int srcChannelNum_, const int dstChannelNum_)
        : srcChannelNum (srcChannelNum_),
          dstChannelNum (dstChannelNum_)
    { }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray <MidiBuffer>&, const int numSamples)
    {
        sharedBufferChans.copyFrom (dstChannelNum, 0, sharedBufferChans, srcChannelNum, 0, numSamples);
    }

private:
    const int srcChannelNum, dstChannelNum;

    JUCE_DECLARE_NON_COPYABLE (CopyChannelOp)
};


class AddChannelOp : public Task
{
public:
    AddChannelOp (const int srcChannelNum_, const int dstChannelNum_)
        : srcChannelNum (srcChannelNum_),
          dstChannelNum (dstChannelNum_)
    { }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray <MidiBuffer>&, const int numSamples)
    {
        sharedBufferChans.addFrom (dstChannelNum, 0, sharedBufferChans, srcChannelNum, 0, numSamples);
    }

private:
    const int srcChannelNum, dstChannelNum;

    JUCE_DECLARE_NON_COPYABLE (AddChannelOp)
};


class ClearMidiBufferOp : public Task
{
public:
    ClearMidiBufferOp (const int bufferNum_)
        : bufferNum (bufferNum_)
    {}

    void perform (AudioSampleBuffer&, const OwnedArray <MidiBuffer>& sharedMidiBuffers, const int)
    {
        sharedMidiBuffers.getUnchecked (bufferNum)->clear();
    }

private:
    const int bufferNum;

    JUCE_DECLARE_NON_COPYABLE (ClearMidiBufferOp)
};


class CopyMidiBufferOp : public Task
{
public:
    CopyMidiBufferOp (const int srcBufferNum_, const int dstBufferNum_)
        : srcBufferNum (srcBufferNum_),
          dstBufferNum (dstBufferNum_)
    { }

    void perform (AudioSampleBuffer&, const OwnedArray <MidiBuffer>& sharedMidiBuffers, const int)
    {
        *sharedMidiBuffers.getUnchecked (dstBufferNum) = *sharedMidiBuffers.getUnchecked (srcBufferNum);
    }

private:
    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (CopyMidiBufferOp)
};


class AddMidiBufferOp : public Task
{
public:
    AddMidiBufferOp (const int srcBufferNum_, const int dstBufferNum_)
        : srcBufferNum (srcBufferNum_),
          dstBufferNum (dstBufferNum_)
    { }

    void perform (AudioSampleBuffer&, const OwnedArray <MidiBuffer>& sharedMidiBuffers, const int numSamples)
    {
        sharedMidiBuffers.getUnchecked (dstBufferNum)
            ->addEvents (*sharedMidiBuffers.getUnchecked (srcBufferNum), 0, numSamples, 0);
    }

private:
    const int srcBufferNum, dstBufferNum;

    JUCE_DECLARE_NON_COPYABLE (AddMidiBufferOp)
};

class DelayChannelOp : public Task
{
public:
    DelayChannelOp (const int channel_, const int numSamplesDelay_)
        : channel (channel_),
          bufferSize (numSamplesDelay_ + 1),
          readIndex (0), writeIndex (numSamplesDelay_)
    {
        buffer.calloc ((size_t) bufferSize);
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray <MidiBuffer>&, const int numSamples)
    {
        float* data = sharedBufferChans.getWritePointer (channel, 0);

        for (int i = numSamples; --i >= 0;)
        {
            buffer [writeIndex] = *data;
            *data++ = buffer [readIndex];

            if (++readIndex  >= bufferSize) readIndex = 0;
            if (++writeIndex >= bufferSize) writeIndex = 0;
        }
    }

private:
    HeapBlock<float> buffer;
    const int channel, bufferSize;
    int readIndex, writeIndex;

    JUCE_DECLARE_NON_COPYABLE (DelayChannelOp)
};


class ProcessBufferOp : public Task
{
public:
    ProcessBufferOp (const GraphNodePtr& node_,
                     const Array <int>& audioChannelsToUse_,
                     const int totalChans_,
                     const int midiBufferToUse_,
                     const Array <int> chans [PortType::Unknown])
        : node (node_),
          processor (node_->getAudioPluginInstance()),
          audioChannelsToUse (audioChannelsToUse_),
          midiChannelsToUse (chans[PortType::Midi]),
          totalChans (jmax (1, totalChans_)),
          numAudioIns (node_->getNumPorts (PortType::Audio, true)),
          numAudioOuts (node_->getNumPorts (PortType::Audio, false)),
          midiBufferToUse (midiBufferToUse_)
    {
        channels.calloc ((size_t) totalChans);

        while (audioChannelsToUse.size() < totalChans)
            audioChannelsToUse.add (0);

        if (chans[PortType::Midi].size() > 0)
            midiBufferToUse = chans[PortType::Midi].getFirst();
    }

    void perform (AudioSampleBuffer& sharedBufferChans, const OwnedArray <MidiBuffer>& sharedMidiBuffers, const int numSamples)
    {
        for (int i = totalChans; --i >= 0;) {
            channels[i] = sharedBufferChans.getWritePointer (audioChannelsToUse.getUnchecked (i), 0);
        }

        AudioSampleBuffer buffer (channels, totalChans, numSamples);
        
        if (! node->isEnabled())
        {
            for (int ch = numAudioIns; ch < numAudioOuts; ++ch)
                buffer.clear (ch, 0, buffer.getNumSamples());
            return;
        }

        if (node->getInputGain() != node->getLastInputGain()) {
            buffer.applyGainRamp (0, numSamples, node->getLastInputGain(), node->getInputGain());
        } else {
            buffer.applyGain(0, numSamples, node->getInputGain());
        }

        for (int i = numAudioIns; --i >= 0;)
            node->setInputRMS (i, buffer.getRMSLevel (i, 0, numSamples));

       #ifndef EL_FREE
        // Begin MIDI filters
        {
            jassert (tempMidi.getNumEvents() == 0);
            ScopedLock spl (node->getPropertyLock());
            transpose.setNoteOffset (node->getTransposeOffset());
            const auto keyRange (node->getKeyRange());
            const auto midiChans (node->getMidiChannels());

            if (keyRange.getLength() > 0 || !midiChans.isOmni())
            {
                auto& midi = *sharedMidiBuffers.getUnchecked (midiBufferToUse);
                MidiBuffer::Iterator iter (midi);
                int frame = 0; MidiMessage msg;
                while (iter.getNextEvent (msg, frame))
                {
                    if (msg.isNoteOnOrOff())
                    {
                        // out of range 
                        if (keyRange.getLength() > 0 && (msg.getNoteNumber() < keyRange.getStart() || msg.getNoteNumber() > keyRange.getEnd()))
                            continue;
                    }

                    if (msg.getChannel() > 0 && midiChans.isOff (msg.getChannel()))
                        continue;

                    transpose.process (msg);
                    tempMidi.addEvent (msg, frame);
                }

                midi.swapWith (tempMidi);
            }
            else
            {
                transpose.process (*sharedMidiBuffers.getUnchecked (midiBufferToUse), numSamples);
            }
        }
        tempMidi.clear();
        // End MIDI filters
       #endif
        
        if (node->wantsMidiPipe())
        {
            MidiPipe midiPipe (sharedMidiBuffers, midiChannelsToUse);
            if (! node->isSuspended())
                node->render (buffer, midiPipe);
            else
                node->renderBypassed (buffer, midiPipe);
        }
        else
        {
            if (! processor->isSuspended ())
            {
                processor->processBlock (buffer, *sharedMidiBuffers.getUnchecked (midiBufferToUse));
            }
            else
            {
                processor->processBlockBypassed (buffer, *sharedMidiBuffers.getUnchecked (midiBufferToUse));
            }
        }
        
        if (node->getGain() != node->getLastGain()) {
            buffer.applyGainRamp (0, numSamples, node->getLastGain(), node->getGain());
        } else {
            buffer.applyGain (0, numSamples, node->getGain());
        }

        node->updateGain();

        for (int i = 0; i < numAudioOuts; ++i)
            node->setOutputRMS (i, buffer.getRMSLevel (i, 0, numSamples));
    }

    const GraphNodePtr node;
    AudioProcessor* const processor;

private:
    Array <int> audioChannelsToUse;
    Array <int> midiChannelsToUse;
    HeapBlock <float*> channels;
    int totalChans, numAudioIns, numAudioOuts;
    int midiBufferToUse;
    MidiTranspose transpose;
    MidiBuffer tempMidi;
    JUCE_DECLARE_NON_COPYABLE (ProcessBufferOp)
};


/** Used to calculate the correct sequence of rendering ops needed, based on
    the best re-use of shared buffers at each stage. */
class ProcessorGraphBuilder
{
public:
    ProcessorGraphBuilder (GraphProcessor& graph_, 
                           const Array<void*>& orderedNodes_,
                           Array<void*>& renderingOps)
        : graph (graph_),
          orderedNodes (orderedNodes_),
          totalLatency (0)
    {
        for (int i = 0; i < PortType::Unknown; ++i)
        {
            allNodes[i].add ((uint32) zeroNodeID);  // first buffer is read-only zeros
            allPorts[i].add (KV_INVALID_PORT);
        }

        for (int i = 0; i < orderedNodes.size(); ++i)
        {
            createRenderingOpsForNode ((GraphNode*) orderedNodes.getUnchecked (i),
                                       renderingOps, i);
            markUnusedBuffersFree (i);
        }

        graph.setLatencySamples (totalLatency);
    }

    int32 buffersNeeded (PortType type)     { return allNodes[type.id()].size(); }

private:
    //==============================================================================
    GraphProcessor& graph;
    const Array<void*>& orderedNodes;
    Array <uint32> allNodes [PortType::Unknown];
    Array <uint32> allPorts [PortType::Unknown];

    enum { freeNodeID = 0xffffffff, zeroNodeID = 0xfffffffe, anonymousNodeID = 0xfffffffd };

    static bool isNodeBusy (uint32 nodeID) noexcept { return nodeID != freeNodeID && nodeID != zeroNodeID; }

    Array <uint32> nodeDelayIDs;
    Array <int> nodeDelays;
    int totalLatency;

    int getNodeDelay (const uint32 nodeID) const          { return nodeDelays [nodeDelayIDs.indexOf (nodeID)]; }

    void setNodeDelay (const uint32 nodeID, const int latency)
    {
        const int index = nodeDelayIDs.indexOf (nodeID);

        if (index >= 0)
        {
            nodeDelays.set (index, latency);
        }
        else
        {
            nodeDelayIDs.add (nodeID);
            nodeDelays.add (latency);
        }
    }

    int getInputLatency (const uint32 nodeID) const
    {
        int maxLatency = 0;

        for (int i = graph.getNumConnections(); --i >= 0;)
        {
            const auto* const c = graph.getConnection (i);
            if (c->destNode == nodeID)
                maxLatency = jmax (maxLatency, getNodeDelay (c->sourceNode));
        }

        return maxLatency;
    }

    void createRenderingOpsForNode (GraphNode* const node, Array<void*>& renderingOps,
                                    const int ourRenderingIndex)
    {
        AudioProcessor* const proc (node->getAudioProcessor());

        // don't add IONodes that cannot process
        typedef GraphProcessor::AudioGraphIOProcessor IOProc;
        if (IOProc* ioproc = dynamic_cast<IOProc*> (proc))
        {
            const uint32 numOuts = node->getNumPorts (PortType::Audio, false);
            if (IOProc::audioInputNode == ioproc->getType() && numOuts <= 0)
                return;
            const uint32 numIns = node->getNumPorts (PortType::Audio, true);
            if (IOProc::audioOutputNode == ioproc->getType() && numIns <= 0)
                return;
        }
        
        Array <int> channelsToUse [PortType::Unknown];
        int maxLatency = getInputLatency (node->nodeId);

        const uint32 numPorts (node->getNumPorts());
        for (uint32 port = 0; port < numPorts; ++port)
        {
            const PortType portType (node->getPortType (port));
            if (portType != PortType::Audio && portType != PortType::Midi)
                continue;

            const uint32 numIns    = node->getNumPorts (portType, true);
            const uint32 numOuts   = node->getNumPorts (portType, false);

            // Outputs only need a buffer if the channel index is greater
            // than or equal to the total inputs of the same port type
            if (node->isPortOutput (port))
            {
                const int outputChan = node->getChannelPort (port);
                if (outputChan >= (int)numIns && outputChan < (int)numOuts)
                {
                    const int bufIndex = getFreeBuffer (portType);
                    channelsToUse [portType.id()].add (bufIndex);
                    const uint32 outPort = node->getNthPort (portType, outputChan, false, false);

                    jassert (bufIndex != 0);
                    jassert (outPort == port);
                    jassert (outPort < node->getNumPorts());

                    markBufferAsContaining (bufIndex, portType, node->nodeId, outPort);
                }
                continue;
            }

            jassert (node->isPortInput (port));

            const int inputChan = node->getChannelPort (port);

            // get a list of all the inputs to this node
            Array <uint32> sourceNodes;
            Array <uint32> sourcePorts;
            for (int i = graph.getNumConnections(); --i >= 0;)
            {
                const GraphProcessor::Connection* const c = graph.getConnection (i);

                if (c->destNode == node->nodeId && c->destPort == port)
                {
                    sourceNodes.add (c->sourceNode);
                    sourcePorts.add (c->sourcePort);
                }
            }

            int bufIndex = -1;
            if (sourceNodes.size() == 0)
            {
                // unconnected input channel
                if (portType == PortType::Audio && inputChan >= (int)numOuts)
                {
                    bufIndex = getReadOnlyEmptyBuffer();
                    jassert (bufIndex >= 0);
                }
                else
                {
                    bufIndex = getFreeBuffer (portType);
                    switch (portType.id())
                    {
                        case PortType::Audio:
                            renderingOps.add (new ClearChannelOp (bufIndex));
                            break;
                        case PortType::Midi:
                            renderingOps.add (new ClearMidiBufferOp (bufIndex));
                            break;
                        default:
                            break;
                    }
                }
            }
            else if (sourceNodes.size() == 1)
            {
                // port with a straight forward single input..
                const uint32 srcNode = sourceNodes.getUnchecked (0);
                const uint32 srcPort = sourcePorts.getUnchecked (0);

                bufIndex = getBufferContaining (portType, srcNode, srcPort);

                if (bufIndex < 0)
                {
                    // if not found, this is probably a feedback loop
                    bufIndex = getReadOnlyEmptyBuffer();
                    jassert (bufIndex >= 0);
                }
                
                const bool bufNeededLater = isBufferNeededLater (ourRenderingIndex, port, srcNode, srcPort);
                if (bufNeededLater && (inputChan < (int) numOuts || portType == PortType::Midi))
                {
                    // can't mess up this channel because it's needed later by another node, so we
                    // need to use a copy of it..
                    const int newFreeBuffer = getFreeBuffer (portType);

                    switch (portType.id())
                    {
                        case PortType::Audio:
                            renderingOps.add (new CopyChannelOp (bufIndex, newFreeBuffer));
                            break;
                        case PortType::Midi:
                            renderingOps.add (new CopyMidiBufferOp (bufIndex, newFreeBuffer));
                            break;
                        default:
                            break;
                    }

                    bufIndex = newFreeBuffer;
                }

                const int nodeDelay = getNodeDelay (srcNode);

                if (nodeDelay < maxLatency)
                    renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay));
            }
            else
            {
                // channel with a mix of several inputs..
                // try to find a re-usable channel from our inputs..
                int reusableInputIndex = -1;

                for (int i = 0; i < sourceNodes.size(); ++i)
                {
                    const int sourceBufIndex = getBufferContaining (portType, sourceNodes.getUnchecked(i),
                                                                              sourcePorts.getUnchecked(i));

                    if (sourceBufIndex >= 0
                        && ! isBufferNeededLater (ourRenderingIndex,
                                                  inputChan,
                                                  sourceNodes.getUnchecked(i),
                                                  sourcePorts.getUnchecked(i)))
                    {
                        // we've found one of our input chans that can be re-used..
                        reusableInputIndex = i;
                        bufIndex = sourceBufIndex;

                        if (portType == PortType::Audio)
                        {
                            const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (i));
                            if (nodeDelay < maxLatency)
                                renderingOps.add (new DelayChannelOp (sourceBufIndex, maxLatency - nodeDelay));
                        }

                        break;
                    }
                }

                if (reusableInputIndex < 0)
                {
                    // can't re-use any of our input chans, so get a new one and copy everything into it..
                    bufIndex = getFreeBuffer (portType);
                    jassert (bufIndex != 0);
                    
                    markBufferAsContaining (bufIndex, portType, anonymousNodeID, 0);
                    
                    const int srcIndex = getBufferContaining (portType, sourceNodes.getUnchecked (0),
                                                                        sourcePorts.getUnchecked (0));
                    if (srcIndex < 0)
                    {
                        // if not found, this is probably a feedback loop
                        if (portType == PortType::Audio)
                            renderingOps.add (new ClearChannelOp (bufIndex));
                        else if (portType == PortType::Midi)
                            renderingOps.add (new ClearMidiBufferOp (bufIndex));
                    }
                    else
                    {
                        if (portType == PortType::Audio)
                            renderingOps.add (new CopyChannelOp (srcIndex, bufIndex));
                        else if (portType == PortType::Midi)
                            renderingOps.add (new CopyMidiBufferOp (srcIndex, bufIndex));
                    }

                    reusableInputIndex = 0;

                    if (portType == PortType::Audio)
                    {
                        const int nodeDelay = getNodeDelay (sourceNodes.getFirst());
                        if (nodeDelay < maxLatency)
                            renderingOps.add (new DelayChannelOp (bufIndex, maxLatency - nodeDelay));
                    }
                }

                for (int j = 0; j < sourceNodes.size(); ++j)
                {
                    if (j != reusableInputIndex)
                    {
                        int srcIndex = getBufferContaining (portType, sourceNodes.getUnchecked(j),
                                                                      sourcePorts.getUnchecked(j));
                        if (srcIndex >= 0)
                        {
                            if (portType == PortType::Audio)
                            {
                                const int nodeDelay = getNodeDelay (sourceNodes.getUnchecked (j));

                                if (nodeDelay < maxLatency)
                                {
                                    if (! isBufferNeededLater (ourRenderingIndex, port,
                                                               sourceNodes.getUnchecked(j),
                                                               sourcePorts.getUnchecked(j)))
                                    {
                                        renderingOps.add (new DelayChannelOp (srcIndex, maxLatency - nodeDelay));
                                    }
                                    else // buffer is reused elsewhere, can't be delayed
                                    {
                                        const int bufferToDelay = getFreeBuffer (PortType::Audio);
                                        renderingOps.add (new CopyChannelOp (srcIndex, bufferToDelay));
                                        renderingOps.add (new DelayChannelOp (bufferToDelay, maxLatency - nodeDelay));
                                        srcIndex = bufferToDelay;
                                    }
                                }

                                renderingOps.add (new AddChannelOp (srcIndex, bufIndex));
                            }
                            else if (portType == PortType::Midi)
                            {
                                renderingOps.add (new AddMidiBufferOp (srcIndex, bufIndex));
                            }
                        }
                    }
                }
            }

            jassert (bufIndex >= 0);
            channelsToUse[portType.id()].add (bufIndex);

            if (inputChan < (int) numOuts)
            {
                const int outputPort = node->getNthPort (portType, inputChan, false, false);
                markBufferAsContaining (bufIndex, portType, node->nodeId, outputPort);
            }
        } /* foreach port */

        setNodeDelay (node->nodeId, maxLatency + node->getLatencySamples());
        
        if (node->isAudioIONode() && node->getNumPorts (PortType::Audio, false) == 0)
            totalLatency = maxLatency;

        int totalChans = jmax (node->getNumPorts (PortType::Audio, true),
                               node->getNumPorts (PortType::Audio, false));
        renderingOps.add (new ProcessBufferOp (node, channelsToUse [PortType::Audio],
                                               totalChans, 0, channelsToUse));
    }

    int getFreeBuffer (PortType type)
    {
        jassert (type.id() < PortType::Unknown);

        Array<uint32>& nodes = allNodes [type.id()];
        for (int i = 1; i < nodes.size(); ++i)
            if (nodes.getUnchecked(i) == freeNodeID)
                return i;

        nodes.add ((uint32) freeNodeID);
        return nodes.size() - 1;
    }

    int32 getReadOnlyEmptyBuffer() const noexcept
    {
        return 0;
    }

    int32 getBufferContaining (const PortType type, const uint32 nodeId, const uint32 outputPort) noexcept
    {
        Array<uint32>& nodes = allNodes [type.id()];
        Array<uint32>& ports = allPorts [type.id()];

        for (int i = nodes.size(); --i >= 0;)
            if (nodes.getUnchecked(i) == nodeId
                 && ports.getUnchecked(i) == outputPort)
                return i;

        return -1;
    }

    void markUnusedBuffersFree (const int stepIndex)
    {
        for (uint32 type = 0; type < PortType::Unknown; ++type)
        {
            Array<uint32>& nodes = allNodes [type];
            Array<uint32>& ports = allPorts [type];

            for (int i = 0; i < nodes.size(); ++i)
            {
                if (isNodeBusy (nodes.getUnchecked (i))
                     && ! isBufferNeededLater (stepIndex, KV_INVALID_PORT,
                                                          nodes.getUnchecked(i),
                                                          ports.getUnchecked(i)))
                {
                    nodes.set (i, (uint32) freeNodeID);
                }
            }
        }
    }

    bool isBufferNeededLater (int stepIndexToSearchFrom, uint32 inputChannelOfIndexToIgnore,
                              const uint32 sourceNode, const uint32 outputPortIndex) const
    {
        while (stepIndexToSearchFrom < orderedNodes.size())
        {
            const GraphNode* const node = (const GraphNode*) orderedNodes.getUnchecked (stepIndexToSearchFrom);

            {
                for (uint32 port = 0; port < node->getNumPorts(); ++port)
                {
                    if (port != inputChannelOfIndexToIgnore &&
                          graph.getConnectionBetween (sourceNode, outputPortIndex, node->nodeId, port) != nullptr)
                    {
                        return true;
                    }
                }
            }

            inputChannelOfIndexToIgnore = KV_INVALID_PORT;
            ++stepIndexToSearchFrom;
        }

        return false;
    }

    void markBufferAsContaining (int bufferNum, PortType type, uint32 nodeId, uint32 portIndex)
    {
        Array<uint32>& nodes = allNodes [type.id()];
        Array<uint32>& ports = allPorts [type.id()];

        jassert (bufferNum >= 0 && bufferNum < nodes.size());
        nodes.set (bufferNum, nodeId);
        ports.set (bufferNum, portIndex);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorGraphBuilder)
};

}

GraphProcessor::Connection::Connection (const uint32 sourceNode_, const uint32 sourcePort_,
                                        const uint32 destNode_, const uint32 destPort_) noexcept
    : Arc (sourceNode_, sourcePort_, destNode_, destPort_)
{
    arc = ValueTree (Tags::arc);
    arc.setProperty (Tags::sourceNode, (int) sourceNode, nullptr)
       .setProperty (Tags::sourcePort, (int) sourcePort, nullptr)
       .setProperty (Tags::destNode, (int) destNode, nullptr)
       .setProperty (Tags::destPort, (int) destPort, nullptr);
}
    
GraphProcessor::GraphProcessor()
    : lastNodeId (0),
      renderingBuffers (1, 1),
      currentAudioInputBuffer (nullptr),
      currentAudioOutputBuffer (1, 1),
      currentMidiInputBuffer (nullptr)
{
    for (int i = 0; i < AudioGraphIOProcessor::numDeviceTypes; ++i)
        ioNodes[i] = KV_INVALID_PORT;
}

GraphProcessor::~GraphProcessor()
{
    renderingSequenceChanged.disconnect_all_slots();
    clearRenderingSequence();
    clear();
}

const String GraphProcessor::getName() const
{
    return "Graph";
}

void GraphProcessor::clear()
{
    nodes.clear();
    connections.clear();
    //triggerAsyncUpdate();
    handleAsyncUpdate();
}

GraphNode* GraphProcessor::getNodeForId (const uint32 nodeId) const
{
    for (int i = nodes.size(); --i >= 0;)
        if (nodes.getUnchecked(i)->nodeId == nodeId)
            return nodes.getUnchecked(i);

    return nullptr;
}

GraphNode* GraphProcessor::createNode (uint32 nodeId, AudioProcessor* proc)
{
    return new AudioProcessorNode (nodeId, proc);
}

GraphNode* GraphProcessor::addNode (AudioProcessor* const newProcessor, uint32 nodeId)
{
    if (newProcessor == nullptr || (void*)newProcessor == (void*)this)
    {
        jassertfalse;
        return nullptr;
    }

    for (int i = nodes.size(); --i >= 0;)
    {
        if (nodes.getUnchecked(i)->getAudioProcessor() == newProcessor)
        {
            jassertfalse; // Cannot add the same object to the graph twice!
            return nullptr;
        }
    }

    if (nodeId == 0)
    {
        nodeId = ++lastNodeId;
    }
    else
    {
        // you can't add a node with an id that already exists in the graph..
        jassert (getNodeForId (nodeId) == nullptr);
        removeNode (nodeId);

        if (nodeId > lastNodeId)
            lastNodeId = nodeId;
    }

    newProcessor->setPlayHead (getPlayHead());
    if (auto* iop = dynamic_cast<AudioGraphIOProcessor*> (newProcessor))
        iop->setParentGraph (this);
    
    if (GraphNode* node = createNode (nodeId, newProcessor))
    {
        node->setParentGraph (this);
        node->resetPorts();
        node->prepare (getSampleRate(), getBlockSize(), this);
        nodes.add (node);
        triggerAsyncUpdate();
        return node;
    }
    
    return nullptr;
}

GraphNode* GraphProcessor::addNode (GraphNode* newNode)
{
    if (newNode == nullptr || (void*)newNode->getAudioProcessor() == (void*)this)
    {
        jassertfalse;
        return nullptr;
    }

    for (int i = nodes.size(); --i >= 0;)
    {
        if (nodes.getUnchecked(i).get() == newNode)
        {
            jassertfalse; // Cannot add the same object to the graph twice!
            return nullptr;
        }
    }

    if (newNode->nodeId == 0 || newNode->nodeId == KV_INVALID_NODE)
    {
        const_cast<uint32&>(newNode->nodeId) = ++lastNodeId;
        jassert (newNode->nodeId == lastNodeId);
    }
    else
    {
        // you can't add a node with an id that already exists in the graph..
        jassert (getNodeForId (newNode->nodeId) == nullptr);
        removeNode (newNode->nodeId);

        if (newNode->nodeId > lastNodeId)
            lastNodeId = newNode->nodeId;
    }

    // TODO: playhead in Graph Node base
    // newNode->setPlayHead (getPlayHead());
    
    newNode->setParentGraph (this);
    newNode->resetPorts();
    newNode->prepare (getSampleRate(), getBlockSize(), this);
    triggerAsyncUpdate();
    return nodes.add (newNode);
}

bool GraphProcessor::removeNode (const uint32 nodeId)
{
    disconnectNode (nodeId);

    for (int i = nodes.size(); --i >= 0;)
    {
        GraphNodePtr n = nodes.getUnchecked (i);
        if (nodes.getUnchecked(i)->nodeId == nodeId)
        {
            nodes.remove (i);
         
            // triggerAsyncUpdate();
            // do this syncronoously so it wont try processing with a null graph
            handleAsyncUpdate();
            n->setParentGraph (nullptr);

            if (auto* sub = dynamic_cast<SubGraphProcessor*> (n->getAudioProcessor()))
            {
                DBG("[EL] sub graph removed");
            }

            return true;
        }
    }

    return false;
}

const GraphProcessor::Connection*
GraphProcessor::getConnectionBetween (const uint32 sourceNode,
                                      const uint32 sourcePort,
                                      const uint32 destNode,
                                      const uint32 destPort) const
{
    const Connection c (sourceNode, sourcePort, destNode, destPort);
    ArcSorter sorter;
    return connections [connections.indexOfSorted (sorter, &c)];
}

bool GraphProcessor::isConnected (const uint32 sourceNode,
                                  const uint32 destNode) const
{
    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked(i);

        if (c->sourceNode == sourceNode
             && c->destNode == destNode)
        {
            return true;
        }
    }

    return false;
}

bool GraphProcessor::canConnect (const uint32 sourceNode, const uint32 sourcePort,
                                 const uint32 destNode, const uint32 destPort) const
{
    if (sourceNode == destNode)
        return false;

    const GraphNode* const source = getNodeForId (sourceNode);
    if (source == nullptr)
    {
        DBG("[EL] source is nullptr");
        return false;
    }

    if (sourcePort >= source->getNumPorts())
    {
        DBG("[EL] source port greater than total: port: " 
            << (int) sourcePort << " total: " 
            << (int) source->getNumPorts());
        return false;
    }

    if (! source->isPortOutput (sourcePort))
    {
        DBG("[EL] " << source->getAudioProcessor()->getName() << " source port is not an output port: total: " << (int) source->getNumPorts());
        return false;
    }
    
    const GraphNode* const dest = getNodeForId (destNode);
    
    if (dest == nullptr
         || (destPort >= dest->getNumPorts())
         || (! dest->isPortInput (destPort)))
    {
        return false;
    }

    const PortType sourceType (source->getPortType (sourcePort));
    const PortType destType (dest->getPortType (destPort));

    if (! sourceType.canConnect (destType))
        return false;

    return getConnectionBetween (sourceNode, sourcePort, destNode, destPort) == nullptr;
}

bool GraphProcessor::addConnection (const uint32 sourceNode, const uint32 sourcePort,
                                    const uint32 destNode, const uint32 destPort)
{
    if (! canConnect (sourceNode, sourcePort, destNode, destPort))
        return false;

    ArcSorter sorter;
    Connection* c = new Connection (sourceNode, sourcePort, destNode, destPort);
    connections.addSorted (sorter, c);
    triggerAsyncUpdate();
    return true;
}

bool GraphProcessor::connectChannels (PortType type, uint32 sourceNode, int32 sourceChannel,
                                      uint32 destNode, int32 destChannel)
{
    GraphNode* src = getNodeForId (sourceNode);
    GraphNode* dst = getNodeForId (destNode);
    if (! src && ! dst)
        return false;
    return addConnection (src->nodeId, src->getPortForChannel (type, sourceChannel, false),
                          dst->nodeId, dst->getPortForChannel (type, destChannel, true));
}

void GraphProcessor::removeConnection (const int index)
{
    connections.remove (index);
    triggerAsyncUpdate();
}

bool GraphProcessor::removeConnection (const uint32 sourceNode, const uint32 sourcePort,
                                       const uint32 destNode, const uint32 destPort)
{
    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked (i);

        if (c->sourceNode == sourceNode
             && c->destNode == destNode
             && c->sourcePort == sourcePort
             && c->destPort == destPort)
        {
            removeConnection (i);
            doneAnything = true;
        }
    }
    
    return doneAnything;
}

bool GraphProcessor::disconnectNode (const uint32 nodeId)
{
    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked(i);

        if (c->sourceNode == nodeId || c->destNode == nodeId)
        {
            removeConnection (i);
            doneAnything = true;
        }
    }
    
    return doneAnything;
}

bool GraphProcessor::isConnectionLegal (const Connection* const c) const
{
    jassert (c != nullptr);

    const GraphNode* const source = getNodeForId (c->sourceNode);
    const GraphNode* const dest   = getNodeForId (c->destNode);

    return source != nullptr && dest != nullptr
            && source->isPortOutput (c->sourcePort) && dest->isPortInput (c->destPort)
            && source->getPortType (c->sourcePort).canConnect (dest->getPortType (c->destPort))
            && c->sourcePort < source->getNumPorts()
            && c->destPort < dest->getNumPorts();
}

bool GraphProcessor::removeIllegalConnections()
{
    bool doneAnything = false;

    for (int i = connections.size(); --i >= 0;)
    {
        if (! isConnectionLegal (connections.getUnchecked(i)))
        {
            removeConnection (i);
            doneAnything = true;
        }
    }

    return doneAnything;
}

void GraphProcessor::setMidiChannel (const int channel) noexcept
{
    jassert (isPositiveAndBelow (channel, 17));
    if (channel <= 0)
        midiChannels.setOmni (true);
    else
        midiChannels.setChannel (channel);
}

void GraphProcessor::setMidiChannels (const BigInteger channels) noexcept
{
    ScopedLock sl (getCallbackLock());
    midiChannels.setChannels (channels);
}

void GraphProcessor::setMidiChannels (const kv::MidiChannels channels) noexcept
{
    ScopedLock sl (getCallbackLock());
    midiChannels = channels;
}

bool GraphProcessor::acceptsMidiChannel (const int channel) const noexcept
{
    ScopedLock sl (getCallbackLock());
    return midiChannels.isOn (channel);
}

void GraphProcessor::setVelocityCurveMode (const VelocityCurve::Mode mode) noexcept
{
    ScopedLock sl (getCallbackLock());
    velocityCurve.setMode (mode);
}

static void deleteRenderOpArray (Array<void*>& ops)
{
    for (int i = ops.size(); --i >= 0;)
        delete static_cast<GraphRender::Task*> (ops.getUnchecked (i));
    ops.clearQuick();
}

void GraphProcessor::clearRenderingSequence()
{
    Array<void*> oldOps;

    {
        const ScopedLock sl (getCallbackLock());
        renderingOps.swapWith (oldOps);
    }

    deleteRenderOpArray (oldOps);
}

bool GraphProcessor::isAnInputTo (const uint32 possibleInputId,
                                  const uint32 possibleDestinationId,
                                  const int recursionCheck) const
{
    if (recursionCheck > 0)
    {
        for (int i = connections.size(); --i >= 0;)
        {
            const GraphProcessor::Connection* const c = connections.getUnchecked (i);

            if (c->destNode == possibleDestinationId
                 && (c->sourceNode == possibleInputId
                      || isAnInputTo (possibleInputId, c->sourceNode, recursionCheck - 1)))
                return true;
        }
    }

    return false;
}

void GraphProcessor::buildRenderingSequence()
{
    Array<void*> newRenderingOps;
    int numRenderingBuffersNeeded = 2;
    int numMidiBuffersNeeded = 1;

    {
        //XXX:
        MessageManagerLock mml;

        Array<void*> orderedNodes;

        {
            const LookupTable table (connections);

            for (int i = 0; i < nodes.size(); ++i)
            {
                GraphNode* const node = nodes.getUnchecked(i);
                node->prepare (getSampleRate(), getBlockSize(), this);

                int j = 0;
                for (; j < orderedNodes.size(); ++j)
                    if (table.isAnInputTo (node->nodeId, ((GraphNode*) orderedNodes.getUnchecked(j))->nodeId))
                      break;

                orderedNodes.insert (j, node);
            }
        }

        GraphRender::ProcessorGraphBuilder calculator (*this, orderedNodes, newRenderingOps);

        numRenderingBuffersNeeded = calculator.buffersNeeded (PortType::Audio);
        numMidiBuffersNeeded      = calculator.buffersNeeded (PortType::Midi);
    }

    {
        // swap over to the new rendering sequence..
        const ScopedLock sl (getCallbackLock());

        renderingBuffers.setSize (numRenderingBuffersNeeded, 4096);
        renderingBuffers.clear();

        for (int i = midiBuffers.size(); --i >= 0;)
            midiBuffers.getUnchecked(i)->clear();

        while (midiBuffers.size() < numMidiBuffersNeeded)
            midiBuffers.add (new MidiBuffer());

        renderingOps.swapWith (newRenderingOps);
    }

    // delete the old ones..
    deleteRenderOpArray (newRenderingOps);

    renderingSequenceChanged();
}

void GraphProcessor::getOrderedNodes (ReferenceCountedArray<GraphNode>& orderedNodes)
{
    const LookupTable table (connections);
    for (int i = 0; i < nodes.size(); ++i)
    {
        GraphNode* const node = nodes.getUnchecked(i);
        
        int j = 0;
        for (; j < orderedNodes.size(); ++j)
            if (table.isAnInputTo (node->nodeId, ((GraphNode*) orderedNodes.getUnchecked(j))->nodeId))
                break;
        
        orderedNodes.insert (j, node);
    }
}

void GraphProcessor::handleAsyncUpdate()
{
    buildRenderingSequence();
}

void GraphProcessor::prepareToPlay (double sampleRate, int estimatedSamplesPerBlock)
{
    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (jmax (1, getTotalNumOutputChannels()), estimatedSamplesPerBlock);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
    clearRenderingSequence();
    
    if (getSampleRate() != sampleRate || getBlockSize() != estimatedSamplesPerBlock)
    {
        setPlayConfigDetails (getTotalNumInputChannels(), getTotalNumOutputChannels(),
            sampleRate, estimatedSamplesPerBlock);
    }

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->prepare (sampleRate, estimatedSamplesPerBlock, this);

    buildRenderingSequence();
}

void GraphProcessor::releaseResources()
{
    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->unprepare();

    renderingBuffers.setSize (1, 1);
    midiBuffers.clear();

    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (1, 1);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
}

void GraphProcessor::reset()
{
    const ScopedLock sl (getCallbackLock());
    for (auto node : nodes)
        if (auto* const proc = node->getAudioProcessor())
            proc->reset();
}

// MARK: Process Graph

void GraphProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int32 numSamples = buffer.getNumSamples();

    currentAudioInputBuffer = &buffer;
    currentAudioOutputBuffer.setSize (jmax (1, buffer.getNumChannels()), numSamples);
    currentAudioOutputBuffer.clear();
    
    if (midiChannels.isOmni() && velocityCurve.getMode() == VelocityCurve::Linear)
    {
        currentMidiInputBuffer = &midiMessages;
    }
    else
    {
        filteredMidi.clear();
        MidiBuffer::Iterator iter (midiMessages);
        MidiMessage msg; int frame = 0, chan = 0;
        
        while (iter.getNextEvent (msg, frame))
        {
            chan = msg.getChannel();
            if (chan > 0 && midiChannels.isOff (chan))
                continue;

            if (msg.isNoteOn())
            {
               #ifndef EL_FREE
                msg.setVelocity (velocityCurve.process (msg.getFloatVelocity()));
               #endif
            }

            filteredMidi.addEvent (msg, frame);
        }
        
        currentMidiInputBuffer = &filteredMidi;
    }
    
    currentMidiOutputBuffer.clear();

    for (int i = 0; i < renderingOps.size(); ++i)
    {
        GraphRender::Task* const op = static_cast<GraphRender::Task*> (renderingOps.getUnchecked (i));
        op->perform (renderingBuffers, midiBuffers, numSamples);
    }

    for (int i = 0; i < buffer.getNumChannels(); ++i)
        buffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);
    
    midiMessages.clear();
    midiMessages.addEvents (currentMidiOutputBuffer, 0, numSamples, 0);
}

const String GraphProcessor::getInputChannelName (int channelIndex) const
{
    return "Input " + String (channelIndex + 1);
}

const String GraphProcessor::getOutputChannelName (int channelIndex) const
{
    return "Output " + String (channelIndex + 1);
}

bool GraphProcessor::isInputChannelStereoPair (int /*index*/) const    { return true; }
bool GraphProcessor::isOutputChannelStereoPair (int /*index*/) const   { return true; }
bool GraphProcessor::silenceInProducesSilenceOut() const               { return false; }
double GraphProcessor::getTailLengthSeconds() const                    { return 0; }
bool GraphProcessor::acceptsMidi() const   { return true; }
bool GraphProcessor::producesMidi() const  { return true; }
void GraphProcessor::getStateInformation (MemoryBlock& /*destData*/) { }
void GraphProcessor::setStateInformation (const void* /*data*/, int /*sizeInBytes*/) { }

void GraphProcessor::fillInPluginDescription (PluginDescription& d) const
{
    d.name = getName();
    d.uid = d.name.hashCode();
    d.category = "Graphs";
    d.pluginFormatName = "Internal";
    d.manufacturerName = "Kushview, LLC";
    d.version = "1.0";
    d.isInstrument = acceptsMidi();
    d.numInputChannels = getTotalNumInputChannels();
    d.numOutputChannels = getTotalNumOutputChannels();
}
    
// MARK: AudioGraphIOProcessor
    
GraphProcessor::AudioGraphIOProcessor::AudioGraphIOProcessor (const IODeviceType type_)
    : type (type_), graph (nullptr)
{
}

GraphProcessor::AudioGraphIOProcessor::~AudioGraphIOProcessor()
{
}

const String GraphProcessor::AudioGraphIOProcessor::getName() const
{
   #if ! EL_RUNNING_AS_PLUGIN
    if (auto* const root = dynamic_cast<RootGraph*> (getParentGraph()))
    {
        switch (type)
        {
            case audioOutputNode:   return root->getAudioOutputDeviceName(); break;
            case audioInputNode:    return root->getAudioInputDeviceName();  break;
            case midiOutputNode:    return "MIDI Out"; break;
            case midiInputNode:     return "MIDI In"; break;
            default:                break;
        }
    }
   #endif

    switch (type)
    {
        case audioOutputNode:   return "Audio Output"; break;
        case audioInputNode:    return "Audio Input"; break;
        case midiOutputNode:    return "Midi Output"; break;
        case midiInputNode:     return "Midi Input"; break;
        default:                break;
    }

    return String();
}

void GraphProcessor::AudioGraphIOProcessor::fillInPluginDescription (PluginDescription& d) const
{
    d.name = getName();
    d.uid = d.name.hashCode();
    d.category = "I/O Devices";
    d.pluginFormatName = "Internal";
    d.manufacturerName = "Element";
    d.version = "1.0";
    d.isInstrument = false;
    
    switch (static_cast<int> (this->type)) {
        case audioInputNode:  d.fileOrIdentifier = "audio.input"; break;
        case audioOutputNode: d.fileOrIdentifier = "audio.output"; break;
        case midiInputNode:   d.fileOrIdentifier = "midi.input"; break;
        case midiOutputNode:  d.fileOrIdentifier = "midi.output"; break;
    }
    
    d.numInputChannels = getTotalNumInputChannels();
    if (type == audioOutputNode && graph != nullptr)
        d.numInputChannels = graph->getTotalNumInputChannels();

    d.numOutputChannels = getTotalNumOutputChannels();
    if (type == audioInputNode && graph != nullptr)
        d.numOutputChannels = graph->getTotalNumOutputChannels();
}

void GraphProcessor::AudioGraphIOProcessor::prepareToPlay (double, int)
{
    jassert (graph != nullptr);
}

void GraphProcessor::AudioGraphIOProcessor::releaseResources()
{
}

void GraphProcessor::AudioGraphIOProcessor::processBlock (AudioSampleBuffer& buffer,
                                                          MidiBuffer& midiMessages)
{
    jassert (graph != nullptr);

    switch (type)
    {
        case audioOutputNode:
        {
            for (int i = jmin (graph->currentAudioOutputBuffer.getNumChannels(),
                               buffer.getNumChannels()); --i >= 0;)
            {
                graph->currentAudioOutputBuffer.addFrom (i, 0, buffer, i, 0, buffer.getNumSamples());
            }

            break;
        }

        case audioInputNode:
        {
            for (int i = jmin (graph->currentAudioInputBuffer->getNumChannels(),
                               buffer.getNumChannels()); --i >= 0;)
            {
                buffer.copyFrom (i, 0, *graph->currentAudioInputBuffer, i, 0, buffer.getNumSamples());
            }

            break;
        }

        case midiOutputNode:
            graph->currentMidiOutputBuffer.clear();
            graph->currentMidiOutputBuffer.addEvents (midiMessages, 0, buffer.getNumSamples(), 0);
            midiMessages.clear();
            break;

        case midiInputNode:
            midiMessages.clear();
            midiMessages.addEvents (*graph->currentMidiInputBuffer, 0, buffer.getNumSamples(), 0);
            graph->currentMidiInputBuffer->clear();
            break;

        default:
            break;
    }
}

bool GraphProcessor::AudioGraphIOProcessor::silenceInProducesSilenceOut() const
{
    return isOutput();
}

double GraphProcessor::AudioGraphIOProcessor::getTailLengthSeconds() const
{
    return 0;
}

bool GraphProcessor::AudioGraphIOProcessor::acceptsMidi() const
{
    return type == midiOutputNode;
}

bool GraphProcessor::AudioGraphIOProcessor::producesMidi() const
{
    return type == midiInputNode;
}

const String GraphProcessor::AudioGraphIOProcessor::getInputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioOutputNode:   return "Output " + String (channelIndex + 1);
        case midiOutputNode:    return "Midi Output";
        default:                break;
    }

    return String();
}

const String GraphProcessor::AudioGraphIOProcessor::getOutputChannelName (int channelIndex) const
{
    switch (type)
    {
        case audioInputNode:    return "Input " + String (channelIndex + 1);
        case midiInputNode:     return "Midi Input";
        default:                break;
    }

    return String();
}

bool GraphProcessor::AudioGraphIOProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return type == audioInputNode || type == audioOutputNode;
}

bool GraphProcessor::AudioGraphIOProcessor::isOutputChannelStereoPair (int index) const
{
    return isInputChannelStereoPair (index);
}

bool GraphProcessor::AudioGraphIOProcessor::isInput() const   { return type == audioInputNode  || type == midiInputNode; }
bool GraphProcessor::AudioGraphIOProcessor::isOutput() const  { return type == audioOutputNode || type == midiOutputNode; }
#if 1
bool GraphProcessor::AudioGraphIOProcessor::hasEditor() const                  { return false; }
AudioProcessorEditor* GraphProcessor::AudioGraphIOProcessor::createEditor()    { return nullptr; }
#endif
int GraphProcessor::AudioGraphIOProcessor::getNumParameters()                  { return 0; }
const String GraphProcessor::AudioGraphIOProcessor::getParameterName (int)     { return String(); }

float GraphProcessor::AudioGraphIOProcessor::getParameter (int)                { return 0.0f; }
const String GraphProcessor::AudioGraphIOProcessor::getParameterText (int)     { return String(); }
void GraphProcessor::AudioGraphIOProcessor::setParameter (int, float)          { }

int GraphProcessor::AudioGraphIOProcessor::getNumPrograms()                    { return 0; }
int GraphProcessor::AudioGraphIOProcessor::getCurrentProgram()                 { return 0; }
void GraphProcessor::AudioGraphIOProcessor::setCurrentProgram (int)            { }

const String GraphProcessor::AudioGraphIOProcessor::getProgramName (int)       { return String(); }
void GraphProcessor::AudioGraphIOProcessor::changeProgramName (int, const String&) { }

void GraphProcessor::AudioGraphIOProcessor::getStateInformation (MemoryBlock&) { }
void GraphProcessor::AudioGraphIOProcessor::setStateInformation (const void*, int) { }

void GraphProcessor::AudioGraphIOProcessor::setParentGraph (GraphProcessor* const newGraph)
{
    graph = newGraph;
    if (graph != nullptr)
    {
        setPlayConfigDetails (type == audioOutputNode ? graph->getTotalNumOutputChannels() : 0,
                              type == audioInputNode ? graph->getTotalNumInputChannels() : 0,
                              graph->getSampleRate(), graph->getBlockSize());
        updateHostDisplay();
    }
}

}
