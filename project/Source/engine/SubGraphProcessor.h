
#pragma once

#include "ElementApp.h"
#include "engine/GraphProcessor.h"

namespace Element {

class AppController;
class GraphController;
class PluginManager;

class SubGraphProcessor : public GraphProcessor
{
public:
    SubGraphProcessor();
    virtual ~SubGraphProcessor();
    void fillInPluginDescription (PluginDescription& d) const override;
    GraphController* createGraphController (PluginManager&);

private:
    typedef GraphProcessor::AudioGraphIOProcessor IOProcessor;
    GraphNodePtr ioNodes [PortType::Unknown];
    friend class GraphController;
    void createAllIONodes();
};

}
