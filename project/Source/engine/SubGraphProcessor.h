
#pragma once

#include "engine/GraphProcessor.h"

namespace Element {

class SubGraphProcessor : public GraphProcessor
{
public:
    SubGraphProcessor();
    virtual ~SubGraphProcessor();
    
    void fillInPluginDescription (PluginDescription& d) const override;
};

}
