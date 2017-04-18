
#ifndef EL_MESSAGES_H
#define EL_MESSAGES_H

#include "ElementApp.h"

namespace Element {

/** Send this when a plugin needs loaded into the graph */
class LoadPluginMessage : public Message {
public:
    LoadPluginMessage (const PluginDescription& pd) : desc(pd) { }
    const PluginDescription desc;
};
    
}



#endif  // EL_MESSAGES_H
