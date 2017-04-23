
#ifndef EL_MESSAGES_H
#define EL_MESSAGES_H

#include "ElementApp.h"

namespace Element {

/** Send this when adding a connection */
class AddConnectionMessage : public Message {
public:
    AddConnectionMessage (uint32 s, int sc, uint32 d, int dc)
    {
        sourceNode = s; destNode = d;
        sourceChannel = sc; destChannel = dc;
        jassert(useChannels());
        sourcePort = destPort = ELEMENT_INVALID_PORT;
    }
    uint32 sourceNode, sourcePort, destNode, destPort;
    int sourceChannel, destChannel;
    
    inline bool useChannels() const { return sourceChannel >= 0 && destChannel >= 0; }
    inline bool usePorts() const { return !useChannels(); }
};

class RemoveConnectionMessage : public Message {
public:
    RemoveConnectionMessage (uint32 s, int sc, uint32 d, int dc) {
        sourceNode = s; destNode = d;
        sourceChannel = sc; destChannel = dc;
        jassert(useChannels());
        sourcePort = destPort = ELEMENT_INVALID_PORT;
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

}



#endif  // EL_MESSAGES_H
