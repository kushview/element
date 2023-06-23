/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include <element/audioengine.hpp>
#include "engine/graphnode.hpp"
#include <element/node.hpp>

namespace element {

class PluginManager;
class RootGraph;

class GraphManager : public ChangeBroadcaster
{
public:
    static const uint32 invalidNodeId = EL_INVALID_PORT;
    static const int invalidChannel = -1;

    GraphManager (GraphNode&, PluginManager&);
    ~GraphManager();

    /** Returns the controlled graph */
    GraphNode& getGraph() noexcept { return processor; }

    /** Returns true if controlling the given graph model */
    bool isManaging (const Node& model) const { return graph == model.data(); }

    /** Returns the number of nodes on the controlled graph */
    int getNumNodes() const noexcept;

    /** Returns a node by index */
    const ProcessorPtr getNode (const int index) const noexcept;

    /** Returns a node by NodeId */
    const ProcessorPtr getNodeForId (const uint32 uid) const noexcept;

    /** Returns a node model by Node ID */
    const Node getNodeModelForId (const uint32 nodeId) const noexcept;

    /** Find a graph manager (recursive) */
    GraphManager* findGraphManagerForGraph (const Node& graph) const noexcept;

    /** Returns true if this manager contains a node by ID */
    bool contains (const uint32 nodeId) const;

    /** Adds a node for processing */
    uint32 addNode (const Node& node);

    /** Adss a node with a plugin description */
    uint32 addNode (const PluginDescription* desc, double x = 0.0f, double y = 0.0f, uint32 nodeId = 0);

    /** Remove a node by ID */
    void removeNode (const uint32 nodeId);

    /** Disconnect a node from other nodes */
    void disconnectNode (const uint32 nodeId, const bool inputs = true, const bool outputs = true, const bool audio = true, const bool midi = true);

    /** Returns the number of connections on the graph
        DOES NOT include connections tagged as "missing"
     */
    int getNumConnections() const noexcept;
    const GraphNode::Connection* getConnection (const int index) const noexcept;

    const GraphNode::Connection*
        getConnectionBetween (uint32 sourceNode, int sourcePort, uint32 destNode, int destPort) const noexcept;

    bool canConnect (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel) const noexcept;

    bool addConnection (uint32 sourceFilterUID, int sourceFilterChannel, uint32 destFilterUID, int destFilterChannel);

    void removeConnection (const int index);

    void removeConnection (uint32 sourceNode, uint32 sourcePort, uint32 destNode, uint32 destPort);

    void removeIllegalConnections();

    void clear();

    void setNodeModel (const Node& node);
    inline Node getGraphModel() const { return Node (graph, false); }

    void savePluginStates();

    /** Rebuilds the arcs model according to the GraphNode */
    inline void syncArcsModel()
    {
        processor.removeIllegalConnections();
        processorArcsChanged();
    }

    inline bool isLoaded() const { return loaded; }

private:
    PluginManager& pluginManager;
    GraphNode& processor;
    ValueTree graph, arcs, nodes;
    bool loaded = false;

    uint32 lastUID;

    class Binding;
    friend class Binding;
    OwnedArray<Binding> bindings;

    uint32 getNextUID() noexcept;
    inline void changed() { sendChangeMessage(); }
    Processor* createFilter (const PluginDescription* desc, double x = 0.0f, double y = 0.0f, uint32 nodeId = 0);
    Processor* createPlaceholder (const Node& node);

    void setupNode (const ValueTree& data, ProcessorPtr object);

    void processorArcsChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphManager)
};

class RootGraphManager : public GraphManager
{
public:
    RootGraphManager (RootGraph& graph, PluginManager& plugins);
    ~RootGraphManager();

    /** Return the underlying RootGraph processor */
    RootGraph& getRootGraph() const { return root; }

    /** Unload graph nodes without clearing the model */
    void unloadGraph();

private:
    RootGraph& root;
};

} // namespace element
