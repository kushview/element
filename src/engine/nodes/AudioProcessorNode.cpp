
#include "engine/nodes/AudioProcessorNode.h"
#include "engine/BaseProcessor.h"
#include "engine/GraphProcessor.h"

namespace Element {

static void setAudioProcessorNodePropertiesFrom (const PluginDescription& pd, ValueTree& p)
{
    p.setProperty (Tags::format,        pd.pluginFormatName, nullptr);
    p.setProperty (Tags::identifier,    pd.fileOrIdentifier, nullptr);
    if (pd.pluginFormatName == "Element" && pd.fileOrIdentifier == EL_INTERNAL_ID_GRAPH)
        p.setProperty (Tags::type, Tags::graph.toString(), nullptr);
}

void AudioProcessorNode::EnablementUpdater::handleAsyncUpdate()
{
    DBG("[EL] AudioProcessorNode::EnablementUpdater::handleAsyncUpdate()");
    node.setEnabled (! node.isEnabled());
}

AudioProcessorNode::AudioProcessorNode (uint32 nodeId, AudioProcessor* processor)
    : GraphNode (nodeId),
      enablement (*this)
{
    proc = processor;
    jassert (proc != nullptr);
    setLatencySamples (proc->getLatencySamples());
    if (auto* instance = dynamic_cast<AudioPluginInstance*> (proc.get()))
    {
        setAudioProcessorNodePropertiesFrom (instance->getPluginDescription(), metadata);
    }
    else
    {
        jassertfalse; // need a way to identify normal audio processors
    }
}

AudioProcessorNode::~AudioProcessorNode()
{
    enablement.cancelPendingUpdate();
    pluginState.reset();
    proc = nullptr;
}

void AudioProcessorNode::createPorts()
{
    kv::PortList newPorts;

    int index = 0, channel = 0, busIdx = 0;
    for (busIdx = 0; busIdx < proc->getBusCount(true); ++busIdx)
    {
        auto* const bus = proc->getBus (true, busIdx);
        for (int busCh = 0; busCh < bus->getNumberOfChannels(); ++busCh)
        {
            String name = bus->getName(); name << " " << int (channel + 1);
            String symbol = "audio_in_"; symbol << int (channel + 1);
            newPorts.add (PortType::Audio, index, channel, symbol, name, true);
            ++index; ++channel;
        }
    }
    jassert (channel == proc->getTotalNumInputChannels());

    channel = 0;
    for (busIdx = 0; busIdx < proc->getBusCount (false); ++busIdx)
    {
        auto* const bus = proc->getBus (false, busIdx);
        for (int busCh = 0; busCh < bus->getNumberOfChannels(); ++busCh)
        {
            String name = bus->getName(); name << " " << int (channel + 1);
            String symbol = "audio_out_"; symbol << int (channel + 1);
            newPorts.add (PortType::Audio, index, channel, symbol, name, false);
            ++index; ++channel;
        }
    }
    jassert (channel == proc->getTotalNumOutputChannels());

    const auto& params = proc->getParameters();
    for (int i = 0; i < params.size(); ++i)
    {
        auto* const param = params.getUnchecked (i);
        String symbol = "control_"; symbol << i;
        newPorts.add (PortType::Control, index, i, 
                      symbol, param->getName (32),
                      true);
        ++index;
    }
    
    if (proc->acceptsMidi())
    {
        newPorts.add (PortType::Midi, index, 0, "midi_in_0", "MIDI", true);
        ++index;
    }
    
    if (proc->producesMidi())
    {
        newPorts.add (PortType::Midi, index, 0, "midi_out_0", "MIDI", false);
        ++index;
    }

    jassert (index == newPorts.size());
    
    ports.swapWith (newPorts);
}

}
