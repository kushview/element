// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ElementApp.h"
#include <element/processor.hpp>
#include "engine/velocitycurve.hpp"
#include <element/arc.hpp>
#include <element/signals.hpp>

namespace element {

class Context;

class GraphNode : public Processor,
                  private AsyncUpdater
{
public:
    Signal<void()> renderingSequenceChanged;

    GraphNode() = delete;
    /** Creates an empty graph. */
    GraphNode (Context&);

    /** Destructor.
        Any processor objects that have been added to the graph will also be deleted.
    */
    ~GraphNode();

    /** Represents a connection between two channels of two nodes in an GraphNode.

        To create a connection, use GraphNode::addConnection().
    */
    struct Connection : public Arc
    {
    public:
        Connection (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort) noexcept;
        Connection (const ValueTree props);

    private:
        friend class GraphNode;
        JUCE_LEAK_DETECTOR (Connection)
    };

    /** Deletes all nodes and connections from this graph.
        Any Node)bjects in the graph will be removed.
    */
    void clear();

    /** Reset all nodes in the graph */
    void reset();

    /** Returns the number of nodes in the graph. */
    int getNumNodes() const { return nodes.size(); }

    /** Returns a pointer to one of the nodes in the graph.
        This will return nullptr if the index is out of range.
        @see getNodeForId
    */
    Processor* getNode (const int index) const { return nodes[index]; }

    /** Searches the graph for a node with the given ID number and returns it.
        If no such node was found, this returns nullptr.
        @see getNode
    */
    Processor* getNodeForId (const uint32 nodeId) const;

    /** Adds a node to the graph.

        This creates a new node in the graph, for the specified processor. Once you have
        added a processor to the graph, the graph owns it and will delete it later when
        it is no longer needed.

        The optional nodeId parameter lets you specify an ID to use for the node, but
        if the value is already in use, this new node will overwrite the old one.

        If this succeeds, it returns a pointer to the newly-created node.
    */
    Processor* addNode (Processor* newNode, uint32 nodeId = 0);

    /** Deletes a node within the graph which has the specified ID.

        This will also delete any connections that are attached to this node.
    */
    bool removeNode (uint32 nodeId);

    /** Builds an array of ordered nodes */
    void getOrderedNodes (ReferenceCountedArray<Processor>& res);

    /** Returns the number of connections in the graph. */
    int getNumConnections() const { return connections.size(); }

    /** Returns a pointer to one of the connections in the graph. */
    const Connection* getConnection (int index) const { return connections[index]; }

    /** Searches for a connection between some specified channels.
        If no such connection is found, this returns nullptr.
    */
    const Connection* getConnectionBetween (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort) const;

    /** Returns true if there is a connection between any of the channels of
        two specified nodes.
    */
    bool isConnected (uint32 sourceNode, uint32 destNode) const;

    /** Returns true if it would be legal to connect the specified points. */
    bool canConnect (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort) const;

    /** Attempts to connect two specified channels of two nodes.

        If this isn't allowed (e.g. because you're trying to connect a midi channel
        to an audio one or other such nonsense), then it'll return false.
    */
    bool addConnection (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort);

    /** Connect two ports by channel number */
    bool connectChannels (PortType type, uint32 sourceNode, int32 sourceChannel, uint32 destNode, int32 destChannel);

    /** Deletes the connection with the specified index. */
    void removeConnection (int index);

    /** Deletes any connection between two specified points.
        Returns true if a connection was actually deleted.
    */
    bool removeConnection (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort);

    /** Removes all connections from the specified node. */
    bool disconnectNode (uint32 nodeId);

    /** Returns true if the given connection's channel numbers map on to valid
        channels at each end.
        Even if a connection is valid when created, its status could change if
        a node changes its channel config.
    */
    bool isConnectionLegal (const Connection* connection) const;

    /** Performs a sanity checks of all the connections.

        This might be useful if some of the processors are doing things like changing
        their channel counts, which could render some connections obsolete.
    */
    bool removeIllegalConnections();

    /** Set the allowed MIDI channel of this Graph */
    void setMidiChannel (const int channel) noexcept;

    /** Set the allowed MIDI channels of this Graph */
    void setMidiChannels (const BigInteger channels) noexcept;

    /** Set the allowed MIDI channels of this Graph */
    void setMidiChannels (const MidiChannels channels) noexcept;

    /** returns true if this graph is processing the given channel */
    bool acceptsMidiChannel (const int channel) const noexcept;

    /** Set the MIDI curve of this graph */
    void setVelocityCurveMode (const VelocityCurve::Mode) noexcept;

    //==========================================================================
    void prepareToRender (double sampleRate, int estimatedBlockSize) override;
    void releaseResources() override;

    bool wantsContext() const noexcept override { return true; }
    void render (RenderContext&) override;
    void renderBypassed (RenderContext&) override {}

    int getNumPrograms() const override { return 1; }
    int getCurrentProgram() const override { return 0; }
    const String getProgramName (int index) const override { return "program"; }
    void setCurrentProgram (int index) override {}

    void getState (MemoryBlock&) override {}
    void setState (const void*, int sizeInBytes) override {}

    void getPluginDescription (PluginDescription& desc) const override;

    void refreshPorts() override;
    void setPlayHead (AudioPlayHead*) override;

    void setNumPorts (PortType type, int count, bool inputs, bool async = true);

    /** Returns true if the graph is prepared. */
    bool prepared() const noexcept { return _prepared; }

    /** Rebuild rendering ops immediately. */
    void rebuild() noexcept;

protected:
    //==========================================================================
    virtual void preRenderNodes() {}
    virtual void postRenderNodes() {}

    //==========================================================================
    void initialize() override {}

private:
    // TODO: techdebt. to many friend classes.
    friend class GraphPort;
    friend class IONode;
    friend class GraphManager;
    friend class Processor;
    friend class NodeObjectSync;
    friend class Node;
    friend class AudioProcessorNode;

    Context& _context;

    typedef ArcTable<Connection> LookupTable;
    ReferenceCountedArray<Processor> nodes;
    OwnedArray<Connection> connections;
    uint32 ioNodes[10];

    uint32 lastNodeId;
    AudioSampleBuffer renderingBuffers;
    OwnedArray<MidiBuffer> midiBuffers;
    OwnedArray<AtomBuffer> atomBuffers;
    Array<void*> renderingOps;
    bool _prepared = false;

    AudioSampleBuffer* currentAudioInputBuffer;
    AudioSampleBuffer currentAudioOutputBuffer;
    MidiBuffer* currentMidiInputBuffer;
    MidiBuffer currentMidiOutputBuffer;

    MidiChannels midiChannels;
    VelocityCurve velocityCurve;
    MidiBuffer filteredMidi;

    std::atomic<AudioPlayHead*> playhead { nullptr };

    bool customPortsSet = false;
    PortList userPorts;

    CriticalSection seqLock;
    friend class ScriptNode; // workaround so parameter connections work when params change.
    void handleAsyncUpdate() override;
    void clearRenderingSequence();
    void buildRenderingSequence();
    bool isAnInputTo (uint32 possibleInputId, uint32 possibleDestinationId, int recursionCheck) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphNode)
};

} // namespace element
