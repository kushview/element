
#include "engine/SubGraphProcessor.h"

namespace Element
{

SubGraphProcessor::SubGraphProcessor ()
{
    setPlayConfigDetails (2, 2, 44100.f, 512);
    
}

SubGraphProcessor::~SubGraphProcessor() { }

void SubGraphProcessor::createAllIONodes()
{
    for (int i = 0; i < IOProcessor::numDeviceTypes; ++i)
        if (nullptr == ioNodes [i])
            ioNodes[i] = addNode (new IOProcessor ((IOProcessor::IODeviceType) i));
}

void SubGraphProcessor::fillInPluginDescription (PluginDescription& d) const
{
    d.name                = "Graph";
    d.descriptiveName     = "A nested graph";
    d.pluginFormatName    = "Element";
    d.category            = "Utility";
    d.manufacturerName    = "Element";
    d.version             = ProjectInfo::versionString;
    d.fileOrIdentifier    = "element.graph";
    d.uid                 = (d.name + d.fileOrIdentifier).getHexValue32();
    d.isInstrument        = false;
    d.numInputChannels    = getTotalNumInputChannels();
    d.numOutputChannels   = getTotalNumOutputChannels();
    d.hasSharedContainer  = false;
}

}
