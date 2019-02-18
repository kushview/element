
#include "controllers/GraphManager.h"
#include "engine/SubGraphProcessor.h"
#include "Globals.h"

namespace Element
{

SubGraphProcessor::SubGraphProcessor ()
{
    setPlayConfigDetails (2, 2, 44100.f, 512);
}

SubGraphProcessor::~SubGraphProcessor()
{
    clear();
    controller = nullptr;
}

void SubGraphProcessor::initController (PluginManager& plugins)
{
    if (controller != nullptr)
        return;
    controller = new GraphManager (*this, plugins);
}

void SubGraphProcessor::createAllIONodes() { }

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
