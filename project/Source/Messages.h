
#ifndef EL_MESSAGES_H
#define EL_MESSAGES_H

#include "ElementApp.h"
#include "session/Node.h"

namespace Element {

/** Send this to remove a node from the graph */
class RemoveNodeMessage : public Message {
public:
    RemoveNodeMessage (const Node& node) : nodeId (node.getNodeId()) { }
    RemoveNodeMessage (const uint32 _nodeId) : nodeId (_nodeId) { }
    const uint32 nodeId;
};

/** Send this when adding a connection */
class AddConnectionMessage : public Message {
public:
    AddConnectionMessage (uint32 s, int sc, uint32 d, int dc)
    {
        sourceNode = s; destNode = d;
        sourceChannel = sc; destChannel = dc;
        sourcePort = destPort = KV_INVALID_PORT;
        jassert(useChannels());
    }
    
    AddConnectionMessage (uint32 s, uint32 sp, uint32 d, uint32 dp)
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
    LoadPluginMessage (const PluginDescription& d)
        : Message(), description (d) { }
    ~LoadPluginMessage() { }
    const PluginDescription description;
};

class DuplicateNodeMessage : public Message {
public:
    DuplicateNodeMessage (const Node& n)
        : Message(), node (n) { }
    DuplicateNodeMessage() { }
    const Node node;
};

class DisconnectNodeMessage : public Message {
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



#endif  // EL_MESSAGES_H
