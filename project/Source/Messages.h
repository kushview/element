
#ifndef EL_MESSAGES_H
#define EL_MESSAGES_H

#include "ElementApp.h"

namespace Element {

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
