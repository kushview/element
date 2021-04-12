
#include "graph/RootGraph.h"

namespace Element {

RootGraph::RootGraph() { }

void RootGraph::refreshPorts()
{
    setPorts (PortCount()
        .with (PortType::Audio, audioInputNames.size(), audioOutputNames.size())
        .with (PortType::Midi, 1, 1)
        .toPortList()
    );
}

void RootGraph::setPlayConfigFor (DeviceManager& devices)
{
    if (auto* const device = devices.getCurrentAudioDevice())
        setPlayConfigFor (device);
}

void RootGraph::setPlayConfigFor (AudioIODevice *device)
{
    jassert (device);
    
    const int numIns        = device->getActiveInputChannels().countNumberOfSetBits();
    const int numOuts       = device->getActiveOutputChannels().countNumberOfSetBits();
    const int bufferSize    = device->getCurrentBufferSizeSamples();
    const double sampleRate = device->getCurrentSampleRate();

    updateChannelNames (device);
    graphName = device->getName();
    if (graphName.isEmpty())
        graphName = "Device";
    setName (graphName);
    setRenderDetails (sampleRate, bufferSize);
    refreshPorts();
}

void RootGraph::setPlayConfigFor (const DeviceManager::AudioDeviceSetup& setup)
{
    audioOutputNames.clear();
    audioInputNames.clear();
    for (int i = 0; i < setup.inputChannels.countNumberOfSetBits(); ++i)
         audioInputNames.add (String("Audio In ") + String (i + 1));
    for (int i = 0; i < setup.outputChannels.countNumberOfSetBits(); ++i)
        audioOutputNames.add (String("Audio Out ") + String (i + 1));
    graphName = setup.inputDeviceName;
    if (graphName.isEmpty()) graphName = setup.outputDeviceName;
    if (graphName.isEmpty()) graphName = "Device";
    setName (graphName);
    setRenderDetails (setup.sampleRate, setup.bufferSize);
    refreshPorts();
}

const String RootGraph::getInputChannelName (int c) const { return audioInputNames[c]; }
    
const String RootGraph::getOutputChannelName (int c) const { return audioOutputNames[c]; }

void RootGraph::updateChannelNames (AudioIODevice* device)
{
    auto activeIn  = device->getActiveInputChannels();
    auto namesIn   = device->getInputChannelNames();
    auto activeOut = device->getActiveOutputChannels();
    auto namesOut  = device->getOutputChannelNames();
    audioOutputNames.clear();
    audioInputNames.clear();
    for (int i = 0; i < namesIn.size(); ++i)
        if (activeIn[i] == true)
            audioInputNames.add (namesIn[i]);
    for (int i = 0; i < namesOut.size(); ++i)
        if (activeOut[i] == true)
            audioOutputNames.add (namesOut[i]);
}

}
