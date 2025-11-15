// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include <element/audioengine.hpp>
#include <element/midipipe.hpp>
#include <element/node.hpp>
#include <element/portcount.hpp>
#include <element/context.hpp>
#include <element/symbolmap.hpp>

#include "engine/graphbuilder.hpp"
#include "engine/ionode.hpp"
#include "nodes/audioprocessor.hpp"
#include "engine/miditranspose.hpp"
#include "nodes/nodetypes.hpp"
#include "engine/graphnode.hpp"

#ifndef EL_GRAPH_NODE_NAME
#define EL_GRAPH_NODE_NAME "Graph"
#endif

namespace element {

GraphNode::Connection::Connection (const uint32 sourceNode_, const uint32 sourcePort_, const uint32 destNode_, const uint32 destPort_) noexcept
    : Arc (sourceNode_, sourcePort_, destNode_, destPort_) {}

GraphNode::GraphNode (Context& c)
    : Processor (PortCount()
                     .with (PortType::Audio, 2, 2)
                     .with (PortType::Midi, 1, 1)
                     .toPortList()),
      _context (c),
      lastNodeId (0),
      renderingBuffers (1, 1),
      currentAudioInputBuffer (nullptr),
      currentAudioOutputBuffer (1, 1),
      currentMidiInputBuffer (nullptr)
{
    for (int i = 0; i < IONode::numDeviceTypes; ++i)
        ioNodes[i] = EL_INVALID_PORT;
    setName (EL_GRAPH_NODE_NAME);
}

GraphNode::~GraphNode()
{
    renderingSequenceChanged.disconnect_all_slots();
    clearRenderingSequence();
    clear();
}

void GraphNode::clear()
{
    clearRenderingSequence();
    nodes.clear();
    connections.clear();
}

Processor* GraphNode::getNodeForId (const uint32 nodeId) const
{
    for (int i = nodes.size(); --i >= 0;)
        if (nodes.getUnchecked (i)->nodeId == nodeId)
            return nodes.getUnchecked (i);

    return nullptr;
}

Processor* GraphNode::addNode (Processor* newNode, uint32 nodeId)
{
    if (newNode == nullptr || (void*) newNode->getAudioProcessor() == (void*) this)
    {
        jassertfalse;
        return nullptr;
    }

    for (int i = nodes.size(); --i >= 0;)
    {
        if (nodes.getUnchecked (i).get() == newNode)
        {
            jassertfalse; // Cannot add the same object to the graph twice!
            return nullptr;
        }
    }

    if (nodeId == 0 || nodeId == EL_INVALID_NODE)
    {
        const_cast<uint32&> (newNode->nodeId) = ++lastNodeId;
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

        const_cast<uint32&> (newNode->nodeId) = nodeId;
        jassert (newNode->nodeId == nodeId);
        if (nodeId > lastNodeId)
            lastNodeId = nodeId;
    }

    newNode->setPlayHead (playhead);
    newNode->setParentGraph (this);
    newNode->refreshPorts();
    if (prepared())
        newNode->prepare (getSampleRate(), getBlockSize(), this);
    triggerAsyncUpdate();
    return nodes.add (newNode);
}

bool GraphNode::removeNode (const uint32 nodeId)
{
    disconnectNode (nodeId);
    for (int i = nodes.size(); --i >= 0;)
    {
        ProcessorPtr n = nodes.getUnchecked (i);
        if (n->nodeId == nodeId)
        {
            nodes.remove (i);

            handleAsyncUpdate();
            n->setParentGraph (nullptr);
            n->setPlayHead (nullptr);

            if (n->isSubGraph())
            {
                DBG ("[element] sub graph removed");
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
    return connections[connections.indexOfSorted (sorter, &c)];
}

bool GraphNode::isConnected (const uint32 sourceNode,
                             const uint32 destNode) const
{
    for (int i = connections.size(); --i >= 0;)
    {
        const Connection* const c = connections.getUnchecked (i);

        if (c->sourceNode == sourceNode
            && c->destNode == destNode)
        {
            return true;
        }
    }

    return false;
}

bool GraphNode::canConnect (const uint32 sourceNode, const uint32 sourcePort, const uint32 destNode, const uint32 destPort) const
{
    if (sourceNode == destNode)
    {
        DBG ("[element] cannot connect to self: " << (int) sourceNode);
        return false;
    }

    const Processor* const source = getNodeForId (sourceNode);
    if (source == nullptr)
    {
        DBG ("[element] source not found");
        return false;
    }

    if (sourcePort >= source->getNumPorts())
    {
        DBG ("[element] source port greater than total: port: "
             << source->getName() << ": "
             << (int) sourcePort << " total: "
             << (int) source->getNumPorts());
        return false;
    }

    if (! source->isPortOutput (sourcePort))
    {
        DBG ("[element] source port is not an output port: " << (int) sourcePort);
        return false;
    }

    const Processor* const dest = getNodeForId (destNode);

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

    // Graph Builder on understands one-to-one for these configs.
    // - Any to Control
    // - Control to CV
    // clang-format off
    if (destType == PortType::Control || 
        (sourceType == PortType::Control && destType == PortType::CV))
    {
        int numSources = 0;
        for (const auto* c : connections)
            if (c->destNode == destNode && c->destPort == destPort)
                if (++numSources > 0)
                    return false;
    }
    // clang-format on

    return getConnectionBetween (sourceNode, sourcePort, destNode, destPort) == nullptr;
}

bool GraphNode::addConnection (const uint32 sourceNode, const uint32 sourcePort, const uint32 destNode, const uint32 destPort)
{
    if (! canConnect (sourceNode, sourcePort, destNode, destPort))
        return false;

    ArcSorter sorter;
    Connection* c = new Connection (sourceNode, sourcePort, destNode, destPort);
    connections.addSorted (sorter, c);
    triggerAsyncUpdate();
    return true;
}

bool GraphNode::connectChannels (PortType type, uint32 sourceNode, int32 sourceChannel, uint32 destNode, int32 destChannel)
{
    Processor* src = getNodeForId (sourceNode);
    Processor* dst = getNodeForId (destNode);
    if (! src && ! dst)
        return false;
    return addConnection (src->nodeId, src->getPortForChannel (type, sourceChannel, false), dst->nodeId, dst->getPortForChannel (type, destChannel, true));
}

void GraphNode::removeConnection (const int index)
{
    connections.remove (index);
    cancelPendingUpdate();
    triggerAsyncUpdate();
}

bool GraphNode::removeConnection (const uint32 sourceNode, const uint32 sourcePort, const uint32 destNode, const uint32 destPort)
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
        const Connection* const c = connections.getUnchecked (i);

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

    const Processor* const source = getNodeForId (c->sourceNode);
    const Processor* const dest = getNodeForId (c->destNode);

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
        if (! isConnectionLegal (connections.getUnchecked (i)))
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

void GraphNode::setMidiChannels (const MidiChannels channels) noexcept
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
        const ScopedLock sl (seqLock);
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
    int numAtomBuffersNeeded = 1;

    {
        //XXX:
        //MessageManagerLock mml;

        Array<void*> orderedNodes;

        {
            const LookupTable table (connections);

            for (int i = 0; i < nodes.size(); ++i)
            {
                Processor* const node = nodes.getUnchecked (i);

                int j = 0;
                for (; j < orderedNodes.size(); ++j)
                    if (table.isAnInputTo (node->nodeId, ((Processor*) orderedNodes.getUnchecked (j))->nodeId))
                        break;

                orderedNodes.insert (j, node);
            }
        }

        GraphBuilder builder (*this, orderedNodes, newRenderingOps);
        numRenderingBuffersNeeded = builder.buffersNeeded (PortType::Audio);
        numMidiBuffersNeeded = builder.buffersNeeded (PortType::Midi);
        numAtomBuffersNeeded = builder.buffersNeeded (PortType::Atom);
        setLatencySamples (builder.getTotalLatencySamples());
    }

    {
        // swap over to the new rendering sequence..
        {
            const ScopedLock sl (getPropertyLock());
            renderingBuffers.setSize (numRenderingBuffersNeeded, 4096);
            renderingBuffers.clear();
            for (auto ab : atomBuffers)
                ab->clear();
            for (int i = midiBuffers.size(); --i >= 0;)
                midiBuffers.getUnchecked (i)->clear();

            while (midiBuffers.size() < numMidiBuffersNeeded)
                midiBuffers.add (new MidiBuffer());
            while (atomBuffers.size() < numAtomBuffersNeeded)
            {
                auto ab = atomBuffers.add (new AtomBuffer());
                ab->setTypes (_context.symbols());
            }
        }

        ScopedLock sl (seqLock);
        renderingOps.swapWith (newRenderingOps);
    }

    // delete the old ones..
    deleteRenderOpArray (newRenderingOps);

    renderingSequenceChanged();
}

void GraphNode::getOrderedNodes (ReferenceCountedArray<Processor>& orderedNodes)
{
    const LookupTable table (connections);
    for (int i = 0; i < nodes.size(); ++i)
    {
        Processor* const node = nodes.getUnchecked (i);

        int j = 0;
        for (; j < orderedNodes.size(); ++j)
            if (table.isAnInputTo (node->nodeId, ((Processor*) orderedNodes.getUnchecked (j))->nodeId))
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
    const bool paramsChanged = (getSampleRate() != sampleRate || getBlockSize() != estimatedSamplesPerBlock);

    if (prepared() && ! paramsChanged)
        return;

    if (prepared() && paramsChanged)
    {
        releaseResources();
    }

    currentAudioInputBuffer = nullptr;
    currentAudioOutputBuffer.setSize (jmax (1, getNumAudioOutputs()), estimatedSamplesPerBlock);
    currentMidiInputBuffer = nullptr;
    currentMidiOutputBuffer.clear();
    clearRenderingSequence();

    _prepared = true;
    if (paramsChanged)
        setRenderDetails (sampleRate, estimatedSamplesPerBlock);

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked (i)->prepare (sampleRate, estimatedSamplesPerBlock, this);

    buildRenderingSequence();
}

void GraphNode::releaseResources()
{
    if (! prepared())
        return;

    for (int i = 0; i < nodes.size(); ++i)
        nodes.getUnchecked (i)->unprepare();

    _prepared = false;

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

void GraphNode::render (RenderContext& rc)
{
    const int32 numSamples = rc.audio.getNumSamples();
    auto& midiMessages = *rc.midi.getWriteBuffer (0);
    currentAudioInputBuffer = &rc.audio;
    currentAudioOutputBuffer.setSize (jmax (1, rc.audio.getNumChannels()), numSamples);
    currentAudioOutputBuffer.clear();

    if (midiChannels.isOmni() && velocityCurve.getMode() == VelocityCurve::Linear)
    {
        currentMidiInputBuffer = &midiMessages;
    }
    else
    {
        filteredMidi.clear();
        int chan = 0;

        for (auto m : midiMessages)
        {
            auto msg = m.getMessage();
            chan = msg.getChannel();
            if (chan > 0 && midiChannels.isOff (chan))
                continue;

            if (msg.isNoteOn())
            {
                msg.setVelocity (velocityCurve.process (msg.getFloatVelocity()));
            }

            filteredMidi.addEvent (msg, m.samplePosition);
        }

        currentMidiInputBuffer = &filteredMidi;
    }

    currentMidiOutputBuffer.clear();

    {
        ScopedLock sl (seqLock);
        for (auto ptr : renderingOps)
        {
            GraphOp* const op = static_cast<GraphOp*> (ptr);
            op->perform (renderingBuffers, midiBuffers, atomBuffers, numSamples);
        }
    }

    for (int i = 0; i < rc.audio.getNumChannels(); ++i)
        rc.audio.copyFrom (i, 0, currentAudioOutputBuffer, i, 0, numSamples);

    midiMessages.clear();
    midiMessages.addEvents (currentMidiOutputBuffer, 0, numSamples, 0);
}

void GraphNode::getPluginDescription (PluginDescription& d) const
{
    d.name = getName();
    d.uniqueId = d.name.hashCode();
    d.category = "Graphs";
    d.pluginFormatName = EL_NODE_FORMAT_NAME;
    d.fileOrIdentifier = EL_NODE_ID_GRAPH;
    d.manufacturerName = EL_NODE_FORMAT_AUTHOR;
    d.version = "1.0.0";
    d.isInstrument = false;
    d.numInputChannels = getNumAudioInputs();
    d.numOutputChannels = getNumAudioOutputs();
}

void GraphNode::setPlayHead (AudioPlayHead* newPlayHead)
{
    Processor::setPlayHead (newPlayHead);
    playhead = getPlayHead();
    for (auto* const node : nodes)
        node->setPlayHead (playhead);
}

void GraphNode::refreshPorts()
{
    if (! customPortsSet)
    {
        auto count = PortCount().with (PortType::Audio, 2, 2).with (PortType::Midi, 1, 1);
        setPorts (count.toPortList());
    }
    else
    {
        setPorts (userPorts);
    }

    for (auto* node : nodes)
    {
        if (auto io = dynamic_cast<IONode*> (node))
            io->refreshPorts();
    }
}

void GraphNode::setNumPorts (PortType type, int count, bool inputs, bool async)
{
    if (type != PortType::Audio && type != PortType::Midi)
        return;

    count = std::max ((int) 0, count);
    PortCount newCount;
    for (int ti = 0; ti < PortType::Unknown; ++ti)
    {
        auto tp = PortType (ti);
        newCount.set (tp,
                      (inputs && type == tp) ? count : getNumPorts (tp, true),
                      (! inputs && type == tp) ? count : getNumPorts (tp, false));
    }

    userPorts = newCount.toPortList();
    customPortsSet = true;

    if (async)
        triggerPortReset();
    else
    {
        resetPorts();
        portsChanged();
    }
}

SymbolMap& GraphNode::symbols() noexcept { return _context.symbols(); }

void GraphNode::rebuild() noexcept
{
    cancelPendingUpdate();
    handleAsyncUpdate();
}

} // namespace element
