
#pragma once

#include "ElementApp.h"
#include "engine/GraphProcessor.h"

namespace Element {

class AppController;
class GraphManager;
class PluginManager;

class SubGraphProcessor : public GraphProcessor
{
public:
    SubGraphProcessor();
    virtual ~SubGraphProcessor();
    void fillInPluginDescription (PluginDescription& d) const override;
    GraphManager& getController() const { jassert(controller); return* controller; }
    
private:
    typedef GraphProcessor::AudioGraphIOProcessor IOProcessor;
    GraphNodePtr ioNodes [PortType::Unknown];
    friend class GraphManager;
    ScopedPointer<GraphManager> controller;
    void createAllIONodes();
    void initController (PluginManager& plugins);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubGraphProcessor);
};

}
