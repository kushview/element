
#pragma once

#include "JuceHeader.h"

namespace Element {

class ControllerMapHandler;
class GraphNode;

class MappingEngine
{
public:
    MappingEngine();
    ~MappingEngine();

    void startMapping();
    void stopMapping();

private:
    class Inputs;
    std::unique_ptr<Inputs> inputs;
};

}
