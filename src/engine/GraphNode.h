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

#pragma once

#include "ElementApp.h"
#include "engine/NodeObject.h"
#include "engine/VelocityCurve.h"
#include "signals.hpp"

namespace Element {

class GraphNode : public NodeObject,
                  private AsyncUpdater
{
public:
    Signal<void()> renderingSequenceChanged;

    /** Creates an empty graph. */
    GraphNode();

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
    NodeObject* getNode (const int index) const { return nodes[index]; }

    /** Searches the graph for a node with the given ID number and returns it.
        If no such node was found, this returns nullptr.
        @see getNode
    */
    NodeObject* getNodeForId (const uint32 nodeId) const;

    /** Adds a node to the graph.

        This creates a new node in the graph, for the specified processor. Once you have
        added a processor to the graph, the graph owns it and will delete it later when
        it is no longer needed.

        The optional nodeId parameter lets you specify an ID to use for the node, but
        if the value is already in use, this new node will overwrite the old one.

        If this succeeds, it returns a pointer to the newly-created node.
    */
    NodeObject* addNode (NodeObject* newNode, uint32 nodeId = 0);

    /** Deletes a node within the graph which has the specified ID.

        This will also delete any connections that are attached to this node.
    */
    bool removeNode (uint32 nodeId);

    /** Builds an array of ordered nodes */
    void getOrderedNodes (ReferenceCountedArray<NodeObject>& res);

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
    void setMidiChannels (const kv::MidiChannels channels) noexcept;

    /** returns true if this graph is processing the given channel */
    bool acceptsMidiChannel (const int channel) const noexcept;

    /** Set the MIDI curve of this graph */
    void setVelocityCurveMode (const VelocityCurve::Mode) noexcept;

    //==========================================================================
    void prepareToRender (double sampleRate, int estimatedBlockSize) override;
    void releaseResources() override;

    bool wantsMidiPipe() const override { return true; }
    void render (AudioSampleBuffer& audio, MidiPipe& midi) override;
    void renderBypassed (AudioSampleBuffer&, MidiPipe&) override {}

    int getNumPrograms() const override { return 1; }
    int getCurrentProgram() const override { return 0; }
    const String getProgramName (int index) const override { return "program"; }
    void setCurrentProgram (int index) override {}

    void getState (MemoryBlock&) override {}
    void setState (const void*, int sizeInBytes) override {}

    void getPluginDescription (PluginDescription& desc) const override;

    void refreshPorts() override {}

    void setPlayHead (AudioPlayHead*) override;

protected:
    //==========================================================================
    virtual void preRenderNodes() {}
    virtual void postRenderNodes() {}

    //==========================================================================
    void initialize() override {}

private:
    friend class GraphPort;
    friend class IONode;
    friend class GraphManager;
    friend class NodeObject;
    friend class NodeObjectSync;
    friend class Node;

    typedef ArcTable<Connection> LookupTable;
    ReferenceCountedArray<NodeObject> nodes;
    OwnedArray<Connection> connections;
    uint32 ioNodes[10];

    uint32 lastNodeId;
    AudioSampleBuffer renderingBuffers;
    OwnedArray<MidiBuffer> midiBuffers;
    Array<void*> renderingOps;

    AudioSampleBuffer* currentAudioInputBuffer;
    AudioSampleBuffer currentAudioOutputBuffer;
    MidiBuffer* currentMidiInputBuffer;
    MidiBuffer currentMidiOutputBuffer;

    kv::MidiChannels midiChannels;
    VelocityCurve velocityCurve;
    MidiBuffer filteredMidi;

    std::atomic<AudioPlayHead*> playhead { nullptr };

    void handleAsyncUpdate() override;
    void clearRenderingSequence();
    void buildRenderingSequence();
    bool isAnInputTo (uint32 possibleInputId, uint32 possibleDestinationId, int recursionCheck) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphNode)
};

} // namespace Element
