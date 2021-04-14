/*
    This file is part of Element
    Copyright (C) 2021  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "graph/GraphBuilder.h"
#include "graph/IONode.h"
#include "engine/nodes/AudioProcessorNode.h"
#include "engine/AudioEngine.h"
#include "engine/MidiPipe.h"
#include "engine/MidiTranspose.h"
#include "engine/nodes/NodeTypes.h"
#include "session/Node.h"
#include "PortCount.h"

#include "graph/GraphNode.h"

namespace Element {

GraphNode::Connection::Connection (const uint32 sourceNode_, const uint32 sourcePort_,
                                   const uint32 destNode_, const uint32 destPort_) noexcept
    : Arc (sourceNode_, sourcePort_, destNode_, destPort_) {}

GraphNode::GraphNode()
    : NodeObject (PortCount()
        .with (PortType::Audio, 2, 2)
        .with (PortType::Midi, 1, 1)
        .toPortList()),
      lastNodeId (0),
      renderingBuffers (1, 1),
      currentAudioInputBuffer (nullptr),
      currentAudioOutputBuffer (1, 1),
      currentMidiInputBuffer (nullptr)
{
    for (int i = 0; i < IONode::numDeviceTypes; ++i)
        ioNodes[i] = KV_INVALID_PORT;
    setName ("Graph");
}

GraphNode::~GraphNode()
{
    renderingSequenceChanged.disconnect_all_slots();
    clearRenderingSequence();
    clear();
}

void GraphNode::clear()
{
    nodes.clear();
    connections.clear();
    clearRenderingSequence();
}

NodeObject* GraphNode::getNodeForId (const uint32 nodeId) const
{
    for (int i = nodes.size(); --i >= 0;)
        if (nodes.getUnchecked(i)->nodeId == nodeId)
            return nodes.getUnchecked (i);

    return nullptr;
}

NodeObject* GraphNode::addNode (NodeObject* newNode, uint32 nodeId)
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

    if (nodeId == 0 || nodeId == KV_INVALID_NODE)
    {
        const_cast<uint32&>(newNode->nodeId) = ++lastNodeId;
        jassert (newNode->nodeId == lastNodeId);
    }
    else
    {
        if (nullptr != getNodeForId (nodeId))
        {
            // you can't add a node with an id that already exists in the graph..
            jassertfalse;
            removeNode (nodeId);
        }

        const_cast<uint32&>(newNode->nodeId) = nodeId;
        jassert(newNode->nodeId == nodeId);
        if (nodeId > lastNodeId)
            lastNodeId = nodeId;
    }

    newNode->setPlayHead (playhead);
    newNode->setParentGraph (this);
    newNode->refreshPorts();
    newNode->prepare (getSampleRate(), getBlockSize(), this);
    triggerAsyncUpdate();
    return nodes.add (newNode);
}

bool GraphNode::removeNode (const uint32 nodeId)
{
    disconnectNode (nodeId);
    for (int i = nodes.size(); --i >= 0;)
    {
        NodeObjectPtr n = nodes.getUnchecked (i);
        if (n->nodeId == nodeId)
        {
            nodes.remove (i);
         
            handleAsyncUpdate();
            n->setParentGraph (nullptr);
            n->setPlayHead (nullptr);

            if (n->isSubGraph())
            {
                DBG("[EL] sub graph removed");
            }

            return true;
        }
    }

    return false;
}

const GraphNode::Connection*
GraphNode::getConnectionBetween (const uint32 sourceNode,
                                      const uint32 sourcePort,
                                      const uint32 destNode,
                                      const uint32 destPort) const
{
    const Connection c (sourceNode, sourcePort, destNode, destPort);
    ArcSorter sorter;
    return connections [connections.indexOfSorted (sorter, &c)];
}

bool GraphNode::isConnected (const uint32 sourceNode,
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

bool GraphNode::canConnect (const uint32 sourceNode, const uint32 sourcePort,
                                 const uint32 destNode, const uint32 destPort) const
{
    if (sourceNode == destNode)
    {
        DBG("[EL] cannot connect to self: " << (int) sourceNode);
        return false;
    }

    const NodeObject* const source = getNodeForId (sourceNode);
    if (source == nullptr)
    {
        DBG("[EL] source not found");
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
        DBG("[EL] source port is not an output port: " << (int) sourcePort);
        return false;
    }
    
    const NodeObject* const dest = getNodeForId (destNode);
    
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

bool GraphNode::addConnection (const uint32 sourceNode, const uint32 sourcePort,
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

bool GraphNode::connectChannels (PortType type, uint32 sourceNode, int32 sourceChannel,
                                      uint32 destNode, int32 destChannel)
{
    NodeObject* src = getNodeForId (sourceNode);
    NodeObject* dst = getNodeForId (destNode);
    if (! src && ! dst)
        return false;
    return addConnection (src->nodeId, src->getPortForChannel (type, sourceChannel, false),
                          dst->nodeId, dst->getPortForChannel (type, destChannel, true));
}

void GraphNode::removeConnection (const int index)
{
    connections.remove (index);
    triggerAsyncUpdate();
}

bool GraphNode::removeConnection (const uint32 sourceNode, const uint32 sourcePort,
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

bool GraphNode::disconnectNode (const uint32 nodeId)
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

bool GraphNode::isConnectionLegal (const Connection* const c) const
{
    jassert (c != nullptr);

    const NodeObject* const source = getNodeForId (c->sourceNode);
    const NodeObject* const dest   = getNodeForId (c->destNode);

    return source != nullptr && dest != nullptr
            && source->isPortOutput (c->sourcePort) && dest->isPortInput (c->destPort)
            && source->getPortType (c->sourcePort).canConnect (dest->getPortType (c->destPort))
            && c->sourcePort < source->getNumPorts()
            && c->destPort < dest->getNumPorts();
}

bool GraphNode::removeIllegalConnections()
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

void GraphNode::setMidiChannel (const int channel) noexcept
{
    jassert (isPositiveAndBelow (channel, 17));
    if (channel <= 0)
        midiChannels.setOmni (true);
    else
        midiChannels.setChannel (channel);
}

void GraphNode::setMidiChannels (const BigInteger channels) noexcept
{
    ScopedLock sl (getPropertyLock());
    midiChannels.setChannels (channels);
}

void GraphNode::setMidiChannels (const kv::MidiChannels channels) noexcept
{
    ScopedLock sl (getPropertyLock());
    midiChannels = channels;
}

bool GraphNode::acceptsMidiChannel (const int channel) const noexcept
{
    ScopedLock sl (getPropertyLock());
    return midiChannels.isOn (channel);
}

void GraphNode::setVelocityCurveMode (const VelocityCurve::Mode mode) noexcept
{
    ScopedLock sl (getPropertyLock());
    velocityCurve.setMode (mode);
}

static void deleteRenderOpArray (Array<void*>& ops)
{
    for (int i = ops.size(); --i >= 0;)
        delete static_cast<GraphOp*> (ops.getUnchecked (i));
    ops.clearQuick();
}

void GraphNode::clearRenderingSequence()
{
    Array<void*> oldOps;

    {
        const ScopedLock sl (getPropertyLock());
        renderingOps.swapWith (oldOps);
    }

    deleteRenderOpArray (oldOps);
}

bool GraphNode::isAnInputTo (const uint32 possibleInputId,
                                  const uint32 possibleDestinationId,
                                  const int recursionCheck) const
{
    if (recursionCheck > 0)
    {
        for (int i = connections.size(); --i >= 0;)
        {
            const GraphNode::Connection* const c = connections.getUnchecked (i);

            if (c->destNode == possibleDestinationId
                 && (c->sourceNode == possibleInputId
                      || isAnInputTo (possibleInputId, c->sourceNode, recursionCheck - 1)))
                return true;
        }
    }

    return false;
}

void GraphNode::buildRenderingSequence()
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
                NodeObject* const node = nodes.getUnchecked(i);
                // TODO: necessary?
                node->prepare (getSampleRate(), getBlockSize(), this);

                int j = 0;
                for (; j < orderedNodes.size(); ++j)
                    if (table.isAnInputTo (node->nodeId, ((NodeObject*) orderedNodes.getUnchecked(j))->nodeId))
                      break;

                orderedNodes.insert (j, node);
            }
        }

        GraphBuilder builder (*this, orderedNodes, newRenderingOps);
        numRenderingBuffersNeeded = builder.buffersNeeded (PortType::Audio);
        numMidiBuffersNeeded      = builder.buffersNeeded (PortType::Midi);
        setLatencySamples (builder.getTotalLatencySamples());
    }

    {
        // swap over to the new rendering sequence..
        // const ScopedLock sl (getCallbackLock());
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

void GraphNode::getOrderedNodes (ReferenceCountedArray<NodeObject>& orderedNodes)
{
    const LookupTable table (connections);
    for (int i = 0; i < nodes.size(); ++i)
    {
        NodeObject* const node = nodes.getUnchecked(i);
        
        int j = 0;
        for (; j < orderedNodes.size(); ++j)
            if (table.isAnInputTo (node->nodeId, ((NodeObject*) orderedNodes.getUnchecked(j))->nodeId))
                break;
        
        orderedNodes.insert (j, node);
    }
}

void GraphNode::handleAsyncUpdate()
{
    buildRenderingSequence();
}

void GraphNode::prepareToRender (double sampleRate, int estimatedSamplesPerBlock)
{
    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (jmax (1, getNumAudioOutputs()), estimatedSamplesPerBlock);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
    clearRenderingSequence();

    if (getSampleRate() != sampleRate || getBlockSize() != estimatedSamplesPerBlock)
        setRenderDetails (sampleRate, estimatedSamplesPerBlock);

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked(i)->prepare (sampleRate, estimatedSamplesPerBlock, this);

    buildRenderingSequence();
}

void GraphNode::releaseResources()
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

void GraphNode::reset()
{
    const ScopedLock sl (getPropertyLock());
    for (auto node : nodes)
        if (auto* const proc = node->getAudioProcessor())
            proc->reset();
}

// MARK: Process Graph

void GraphNode::render (AudioSampleBuffer& buffer, MidiPipe& midi)
{
    const int32 numSamples = buffer.getNumSamples();
    auto& midiMessages = *midi.getWriteBuffer (0);
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
        GraphOp* const op = static_cast<GraphOp*> (renderingOps.getUnchecked (i));
        op->perform (renderingBuffers, midiBuffers, numSamples);
    }

    for (int i = 0; i < buffer.getNumChannels(); ++i)
        buffer.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);
    
    midiMessages.clear();
    midiMessages.addEvents (currentMidiOutputBuffer, 0, numSamples, 0);
}

void GraphNode::getPluginDescription (PluginDescription& d) const
{
    d.name = getName();
    d.uid = d.name.hashCode();
    d.category = "Graphs";
    d.pluginFormatName = EL_INTERNAL_FORMAT_NAME;
    d.fileOrIdentifier = EL_INTERNAL_ID_GRAPH;
    d.manufacturerName = "Element";
    d.version = "1.0.0";
    d.isInstrument = false;
    d.numInputChannels  = getNumAudioInputs();
    d.numOutputChannels = getNumAudioOutputs();
}

void GraphNode::setPlayHead (AudioPlayHead* newPlayHead)
{
    playhead = newPlayHead;
    for (auto* const node : nodes)
        node->setPlayHead (playhead);
}

}