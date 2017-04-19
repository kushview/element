/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#ifndef ELEMENT_GRAPH_CONTROLLER_H
#define ELEMENT_GRAPH_CONTROLLER_H

#include "controllers/Controller.h"
#include "engine/GraphProcessor.h"

namespace Element {

class FilterInGraph;
class GraphController;
class PluginManager;

/**
    A collection of filters and some connections between them.
 */
class GraphController :  public ChangeBroadcaster,
                         public Controller
{
public:
    static const uint32 invalidNodeId = (uint32)-1;
    typedef GraphNodePtr NodePtr;

    GraphController (GraphProcessor&, PluginManager&);
    ~GraphController();

    GraphProcessor& getGraph() noexcept { return processor; }
    GraphProcessor& graph() { return getGraph(); }

    PluginManager& plugins() { return pluginManager; }

    int getNumFilters() const noexcept;

    const NodePtr getNode (const int index) const noexcept;
    const NodePtr getNodeForId (const uint32 uid) const noexcept;

    uint32 addFilter (const PluginDescription* desc, double x = 0.0f, double y = 0.0f);

    void removeFilter (const uint32 filterUID);
    void disconnectFilter (const uint32 filterUID);

    void removeIllegalConnections();

    void setNodePosition (const int nodeId, double x, double y);
    void getNodePosition (const int nodeId, double& x, double& y) const;

    int getNumConnections() const noexcept;
    const GraphProcessor::Connection* getConnection (const int index) const noexcept;

    const GraphProcessor::Connection* getConnectionBetween (uint32 sourceFilterUID, int sourceFilterChannel,
                                                            uint32 destFilterUID, int destFilterChannel) const noexcept;

    bool canConnect (uint32 sourceFilterUID, int sourceFilterChannel,
                     uint32 destFilterUID, int destFilterChannel) const noexcept;

    bool addConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                        uint32 destFilterUID, int destFilterChannel);

    void removeConnection (const int index);

    void removeConnection (uint32 sourceFilterUID, int sourceFilterChannel,
                           uint32 destFilterUID, int destFilterChannel);

    void clear();

    /** The special channel index used to refer to a filter's midi channel. */
    static const int midiChannelNumber;

private:
    PluginManager& pluginManager;
    GraphProcessor& processor;

    uint32 lastUID;
    uint32 getNextUID() noexcept;
    inline void changed() { sendChangeMessage(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphController)
};
}
#endif   // ELEMENT_GRAPH_CONTROLLER_H
