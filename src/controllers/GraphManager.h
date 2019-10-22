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

#include "controllers/Controller.h"
#include "engine/AudioEngine.h"
#include "engine/GraphProcessor.h"
#include "session/Node.h"

namespace Element {

class FilterInGraph;
class GraphManager;
class PluginManager;

class GraphManager : public ChangeBroadcaster,
                     public Controller
{
public:
    static const uint32 invalidNodeId   = KV_INVALID_PORT;
    static const int invalidChannel     = -1;
    typedef GraphNodePtr NodePtr;

    GraphManager (GraphProcessor&, PluginManager&);
    ~GraphManager();

    GraphProcessor& getGraph() noexcept { return processor; }

    bool isControlling (const Node& g) const { return graph == g.getValueTree(); }

    int getNumFilters() const noexcept;

    const NodePtr getNode (const int index) const noexcept;
    const NodePtr getNodeForId (const uint32 uid) const noexcept;
    const Node getNodeModelForId (const uint32 nodeId) const noexcept {
        return Node (nodes.getChildWithProperty (Tags::id, static_cast<int64> (nodeId)), false);
    }

    uint32 addNode (const Node& node);
    uint32 addFilter (const PluginDescription* desc, double x = 0.0f, double y = 0.0f,
                      uint32 nodeId = 0);

    void removeFilter (const uint32 filterUID);
    void disconnectFilter (const uint32 filterUID, const bool inputs = true, const bool outputs = true,
                                                   const bool audio = true, const bool midi = true);

    /** Returns the number of connections on the graph
        DOES NOT include connections tagged as "missing"
      */
    int getNumConnections() const noexcept;
    const GraphProcessor::Connection* getConnection (const int index) const noexcept;

    const GraphProcessor::Connection*
    getConnectionBetween (uint32 sourceNode, int sourcePort,
                          uint32 destNode, int destPort) const noexcept;

    bool canConnect (uint32 sourceFilterUID, int sourceFilterChannel,
                     uint32 destFilterUID, int destFilterChannel) const noexcept;

    bool addConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                        uint32 destFilterUID, int destFilterChannel);

    void removeConnection (const int index);

    void removeConnection (uint32 sourceNode, uint32 sourcePort,
                           uint32 destNode, uint32 destPort);

    void removeIllegalConnections();

    void clear();

    void setNodeModel (const Node& node);
    inline Node getGraphModel() const { return Node (graph, false); }
    
    void savePluginStates();
    
    /** Rebuilds the arcs model according to the GraphProcessor */
    inline void syncArcsModel()
    {
        processor.removeIllegalConnections();
        processorArcsChanged();
    }
    
    inline bool isLoaded() const { return loaded; }

private:
    PluginManager& pluginManager;
    GraphProcessor& processor;
    ValueTree graph, arcs, nodes;
    bool loaded = false;
    
    uint32 lastUID;
    uint32 getNextUID() noexcept;
    inline void changed() { sendChangeMessage(); }
    GraphNode* createFilter (const PluginDescription* desc, double x = 0.0f, double y = 0.0f,
                             uint32 nodeId = 0);
    GraphNode* createPlaceholder (const Node& node);
    void setupNode (const ValueTree& data, GraphNodePtr object);
    
    void processorArcsChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphManager)
};
    
class RootGraphManager : public GraphManager {
public:
    RootGraphManager (RootGraph& graph, PluginManager& plugins)
        : GraphManager (graph, plugins),
          root (graph)
    { }
    
    ~RootGraphManager() { }
    
    /** REturn the underlying RootGraph processor */
    RootGraph& getRootGraph() const { return root; }
    
    /** Unload graph nodes without clearing the model */
    void unloadGraph();

private:
    RootGraph& root;
};

}
