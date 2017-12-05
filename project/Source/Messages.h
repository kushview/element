
#pragma once

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

/** Send this to add a preset for a node */
class AddPresetMessage : public Message
{
public:
    AddPresetMessage (const Node& n) : node(n) { }
    ~AddPresetMessage() { }
    
    const Node node;
};

/** Send this to remove a node from the current graph */
class RemoveNodeMessage : public Message
{
public:
    RemoveNodeMessage (const Node& node) : nodeId (node.getNodeId()) { }
    RemoveNodeMessage (const uint32 _nodeId) : nodeId (_nodeId) { }
    const uint32 nodeId;
};

/** Send this to add a new connection */
class AddConnectionMessage : public Message
{
public:
    AddConnectionMessage (uint32 s, int sc, uint32 d, int dc)
    {
        sourceNode = s; destNode = d;
        sourceChannel = sc; destChannel = dc;
        sourcePort = destPort = KV_INVALID_PORT;
        jassert (useChannels());
    }
    
    AddConnectionMessage (uint32 s, uint32 sp, uint32 d, uint32 dp)
    {
        sourceNode = s; destNode = d;
        sourcePort = sp; destPort = dp;
        sourceChannel = destChannel = -1;
        jassert (usePorts());
    }
    
    uint32 sourceNode, sourcePort, destNode, destPort;
    int sourceChannel, destChannel;
    inline bool useChannels() const { return sourceChannel >= 0 && destChannel >= 0; }
    inline bool usePorts() const { return !useChannels(); }
};

/** Send this to remove a connection from the graph */
class RemoveConnectionMessage : public Message {
public:
    RemoveConnectionMessage (uint32 s, int sc, uint32 d, int dc) {
        sourceNode = s; destNode = d;
        sourceChannel = sc; destChannel = dc;
        sourcePort = destPort = KV_INVALID_PORT;
        jassert(useChannels());
    }
    
    RemoveConnectionMessage (uint32 s, uint32 sp, uint32 d, uint32 dp)
    {
        sourceNode = s; destNode = d;
        sourcePort = sp; destPort = dp;
        sourceChannel = destChannel = -1;
        jassert(usePorts());
    }
    
    uint32 sourceNode, sourcePort, destNode, destPort;
    int sourceChannel, destChannel;
    
    inline bool useChannels() const { return sourceChannel >= 0 && destChannel >= 0; }
    inline bool usePorts() const { return !useChannels(); }
};

/** Send this when a plugin needs loaded into the graph */
class LoadPluginMessage : public Message {
public:
    LoadPluginMessage (const PluginDescription& pluginDescription, const bool pluginVerified)
        : Message(), description (pluginDescription), verified (pluginVerified) { }
    LoadPluginMessage (const PluginDescription& d, const bool v, const float rx, const float ry)
        : Message(), description (d), relativeX (rx), relativeY (ry), verified (v) { }
    ~LoadPluginMessage() { }
    
    /** Descriptoin of the plugin to load */
    const PluginDescription description;
    
    /** Relative X of the node UI in a graph editor */
    const float relativeX = 0.5f;

    /** Relative X of the node UI in a graph editor */
    const float relativeY = 0.5f;
    
    /** Whether or not this plugin has been vetted yet */
    const bool verified;
};

class DuplicateNodeMessage : public Message {
public:
    DuplicateNodeMessage (const Node& n)
        : Message(), node (n) { }
    DuplicateNodeMessage() { }
    const Node node;
};

class DisconnectNodeMessage : public Message
{
public:
    DisconnectNodeMessage (const Node& n)
        : Message(), node (n) { }
    DisconnectNodeMessage() { }
    const Node node;
};

class FinishedLaunchingMessage : public Message {
public:
    FinishedLaunchingMessage() { }
    ~FinishedLaunchingMessage() { }
};
}
