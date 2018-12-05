
#include "engine/nodes/AudioProcessorNode.h"

namespace Element {

static void setAudioProcessorNodePropertiesFrom (const PluginDescription& pd, ValueTree& p)
{
    p.setProperty (Tags::format,        pd.pluginFormatName, nullptr);
    p.setProperty (Tags::identifier,    pd.fileOrIdentifier, nullptr);
#if 0
    p.setProperty (Slugs::name, pd.name, nullptr);
    if (pd.descriptiveName != pd.name)
        p.setProperty("descriptiveName", pd.descriptiveName, nullptr);
    
    p.setProperty ("uid",          String::toHexString (pd.uid), nullptr);
    p.setProperty ("isInstrument", pd.isInstrument, nullptr);
    p.setProperty ("fileTime",     String::toHexString (pd.lastFileModTime.toMilliseconds()), nullptr);
    
    p.setProperty ("numInputs",    pd.numInputChannels, nullptr);
    p.setProperty ("numOutputs",   pd.numOutputChannels, nullptr);
    p.setProperty ("isShell",      pd.hasSharedContainer, nullptr);
    p.setProperty ("category",     pd.category, nullptr);
    p.setProperty ("manufacturer", pd.manufacturerName, nullptr);
    p.setProperty ("version",      pd.version, nullptr);
#endif
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

   #if 0
    PluginDescription desc;
    getPluginDescription (desc);

    metadata = ValueTree (Tags::node);
    setAudioProcessorNodePropertiesFrom (desc, metadata);
    metadata.setProperty (Slugs::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Slugs::name, proc->getName(), nullptr)
            .setProperty (Slugs::type, getTypeString(), nullptr)
            .setProperty ("numAudioIns", proc->getTotalNumInputChannels(), nullptr)
            .setProperty ("numAudioOuts", proc->getTotalNumOutputChannels(), nullptr);
   #endif
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

    DBG("Plugin: " << proc->getName());
#if 1
    for (busIdx = 0; busIdx < proc->getBusCount(true); ++busIdx)
    {
        auto* const bus = proc->getBus (true, busIdx);
        for (int busCh = 0; busCh < bus->getNumberOfChannels(); ++busCh)
        {
            String name = bus->getName(); name << " " << int (channel + 1);
            String symbol = "audio_in_"; symbol << int (channel + 1);
            newPorts.add (PortType::Audio, index, channel, symbol, name, true);
            DBG("symbol=" << symbol << " index=" << index << " channel=" << channel);
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
            DBG("symbol=" << symbol << " index=" << index << " channel=" << channel);
            ++index; ++channel;
        }
    }
    jassert (channel == proc->getTotalNumOutputChannels());

#else
    for (channel = 0; channel < proc->getTotalNumInputChannels(); ++channel)
    {
        String symbol = "audio_in_"; symbol << channel;
        newPorts.add (PortType::Audio, index, channel, symbol,
                      proc->getInputChannelName (channel), true);
        ++index;
    }


    for (channel = 0; channel < getNumAudioOutputs(); ++channel)
    {
        String symbol = "audio_out_"; symbol << channel;
        newPorts.add (PortType::Audio, index, channel, symbol,
                      proc->getOutputChannelName (channel), false);
        ++index;
    }
#endif

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
