
#pragma once

#include "JuceHeader.h"

namespace Element {

class ControllerDevice;
class ControllerMapHandler;
class GraphNode;

class MappingEngine
{
public:
    MappingEngine();
    ~MappingEngine();

    bool addInput (const ControllerDevice&);
    bool removeInput (const ControllerDevice&);
    bool refreshInput (const ControllerDevice&);
    void clear();
    void startMapping();
    void stopMapping();

private:
    class Inputs;
    std::unique_ptr<Inputs> inputs;
};

}
