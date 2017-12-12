#include "ElementApp.h"
#include "engine/AudioEngine.h"
#include "engine/GraphNode.h"
#include "engine/GraphProcessor.h"
#include "session/Node.h"

namespace Element {
    
static void setNodePropertiesFrom (const PluginDescription& pd, ValueTree& p)
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

GraphNode::GraphNode (const uint32 nodeId_, AudioProcessor* const processor_) noexcept
    : nodeId (nodeId_),
      isPrepared (false),
      metadata (Tags::node)
{
    proc = processor_;
    parent = nullptr;
    gain.set(1.0f); lastGain.set (1.0f);
    inputGain.set(1.0f); lastInputGain.set (1.0f);
    jassert (proc != nullptr);
    
    PluginDescription desc;
    getPluginDescription (desc);
    
    setNodePropertiesFrom (desc, metadata);
    const String type = (nullptr == dynamic_cast<GraphProcessor*> (processor_)) ? "plugin" : "graph";
    metadata.setProperty (Slugs::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Slugs::name, proc->getName(), nullptr)
            .setProperty (Slugs::type, type, nullptr)
            .setProperty ("numAudioIns", getNumAudioInputs(), nullptr)
            .setProperty ("numAudioOuts", getNumAudioOutputs(), nullptr);
    resetPorts();
}

GraphNode::~GraphNode()
{
    proc = nullptr;
}

GraphNode* GraphNode::createForRoot (GraphProcessor* g)
{
    auto* node = new GraphNode (0, g);
    return node;
}
    
void GraphNode::setInputGain (const float f) {
    inputGain.set(f);
}

void GraphNode::setGain (const float f) {
    gain.set(f);
}

void GraphNode::getPluginDescription (PluginDescription& desc)
{
    if (AudioPluginInstance* i = getAudioPluginInstance())
        i->fillInPluginDescription (desc);
}

void GraphNode::connectAudioTo (const GraphNode* other)
{
    jassert (getParentGraph());
    jassert (getParentGraph() == other->getParentGraph());

    GraphProcessor& graph (*getParentGraph());
    AudioPluginInstance* const src = getAudioPluginInstance();
    AudioPluginInstance* const dst = other->getAudioPluginInstance();

    const int totalChans = jmin (getNumAudioOutputs(), other->getNumAudioInputs());
    bool failed = false;
    for (int chan = 0; chan < totalChans; ++chan)
    {
        failed |= graph.addConnection (
            this->nodeId, Processor::getPortForAudioChannel(src, chan, false),
            other->nodeId, Processor::getPortForAudioChannel(dst, chan, true)
        );
    }

    if (failed)
    {
        DBG("  failed " << src->getName() << " to " << dst->getName());
    }
}

bool GraphNode::isAudioIONode() const
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    if (IOP* iop = dynamic_cast<IOP*> (proc.get()))
    {
        return iop->getType() == IOP::audioInputNode ||
               iop->getType() == IOP::audioOutputNode;
    }

    return false;
}

bool GraphNode::isMidiIONode() const
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    if (IOP* iop = dynamic_cast<IOP*> (proc.get()))
        return iop->getType() == IOP::midiInputNode || iop->getType() == IOP::midiOutputNode;
    return false;
}

int GraphNode::getNumAudioInputs() const { return proc ? proc->getTotalNumInputChannels() : 0; }
int GraphNode::getNumAudioOutputs() const { return proc ? proc->getTotalNumOutputChannels() : 0; }

void GraphNode::setInputRMS (int chan, float val)
{
    if (chan < inRMS.size())
        inRMS.getUnchecked(chan)->set(val);
}

void GraphNode::setOutputRMS (int chan, float val)
{
    if (chan < outRMS.size())
        outRMS.getUnchecked(chan)->set(val);
}

bool GraphNode::isSuspended() const { return (proc) ? proc->isSuspended() : false; }

void GraphNode::suspendProcessing (const bool shouldBeSuspended)
{
    if (proc && proc->isSuspended() != shouldBeSuspended)
        proc->suspendProcessing (shouldBeSuspended);
}

bool GraphNode::isGraph() const
{
    return (nullptr != dynamic_cast<GraphProcessor*> (proc.get()));
}
    
PortType GraphNode::getPortType (const uint32 port) const
{
    return Processor::getPortType (proc.get(), port);
}

int GraphNode::getNumPorts (const PortType type, const bool isInput) const
{
    return static_cast<int> (Processor::getNumPorts (proc.get(), type, isInput));
}
    
uint32 GraphNode::getNumPorts() const
{
    return Processor::getNumPorts (proc.get());
}

bool GraphNode::isPortInput (const uint32 port) const
{
    return Processor::isPortInput (proc.get(), port);
}

bool GraphNode::isPortOutput (const uint32 port) const
{
    return ! Processor::isPortInput (proc.get(), port);
}

uint32 GraphNode::getPortForChannel (const PortType type, const int channel, const bool isInput) const
{
    uint32 port = KV_INVALID_PORT;
    
    switch (type)
    {
        case PortType::Audio:
        {
            port = Processor::getPortForAudioChannel (proc.get(), channel, isInput);
        }
        break;
        
        case PortType::Midi:
        {
            port = (uint32)channel + static_cast<uint32> (getNumAudioInputs() + getNumAudioOutputs() + proc->getNumParameters());
            if (! isInput)
                port += getNumPorts (PortType::Midi, true);
        }
        break;
            
        default:
            jassertfalse;
            break;
    }
    
    return port;
}
    
int GraphNode::getChannelPort (const uint32 port) const
{
    jassert (port < getNumPorts());
    
    int channel = 0;
    const bool isInput  = isPortInput (port);
    const PortType type = getPortType (port);
    
    for (uint32 p = 0; p < (uint32) getNumPorts(); ++p)
    {
        const PortType thisPortType = getPortType (p);
        const bool thisPortIsInput = isPortInput (p);
        
        if (type == thisPortType && p == port)
            return channel;
        
        // tally the channel only if type and flow match
        if (type == thisPortType && isInput == thisPortIsInput)
            ++channel;
    }
    
    return -1;
}
    
int GraphNode::getNthPort (const PortType type, const int index, bool isInput, bool oneBased) const
{
    int count = oneBased ? 0 : -1;
    
    jassert (getNumPorts() >= 0);
    uint32 nports = getNumPorts();
    
    for (uint32 port = 0; port < nports; ++port)
    {
        if (type == getPortType (port) && isInput == isPortInput (port))
        {
            if (++count == index) {
                return port;
            }
        }
    }
    
    jassertfalse;
    return KV_INVALID_PORT;
}

uint32 GraphNode::getMidiInputPort() const
{
    return getPortForChannel (PortType::Midi, 0, true);
}

uint32 GraphNode::getMidiOutputPort() const
{
    return getPortForChannel (PortType::Midi, 0, false);;
}

bool GraphNode::isSubgraph() const noexcept
{
    return (dynamic_cast<GraphProcessor*> (proc.get()) != nullptr);
}

void GraphNode::prepare (const double sampleRate, const int blockSize,
                         GraphProcessor* const parentGraph)
{
    parent = parentGraph;
    AudioProcessor* instance = proc.get();
    
    if (! isPrepared)
    {
        isPrepared = true;
        setParentGraph (parentGraph);
        
        instance->setRateAndBufferSizeDetails (sampleRate, blockSize);
        instance->prepareToPlay (sampleRate, blockSize);
        
        // TODO: move model code out of engine code
        if (nullptr != dynamic_cast<IOProcessor*> (instance))
            resetPorts();
        
        inRMS.clearQuick (true);
        for (int i = 0; i < instance->getTotalNumInputChannels(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set(0);
            inRMS.add (avf);
        }

        outRMS.clearQuick (true);
        for (int i = 0; i < instance->getTotalNumOutputChannels(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set(0);
            outRMS.add(avf);
        }
        
        if (metadata.getProperty (Tags::bypass, false))
            instance->suspendProcessing (true);
    }
}

void GraphNode::unprepare()
{
    if (isPrepared)
    {
        isPrepared = false;
        inRMS.clear (true);
        outRMS.clear (true);
        proc->releaseResources();
        parent = nullptr;
    }
}

static void addPortsIONode (GraphNode* node, GraphProcessor::AudioGraphIOProcessor* proc, ValueTree& ports)
{
    int index = 0;
    auto* graph = proc->getParentGraph();
    if (graph == nullptr || proc == nullptr)
        return;
    
    for (int channel = 0; channel < node->getNumAudioInputs(); ++channel)
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Audio).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::input.toString(), nullptr)
            .setProperty (Slugs::name,  graph->getOutputChannelName (channel), nullptr);
        
        ports.addChild (port, index, nullptr);
        ++index;
    }
    
    for (int channel = 0; channel < node->getNumAudioOutputs(); ++channel)
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Audio).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::output.toString(), nullptr)
            .setProperty (Slugs::name,  graph->getInputChannelName (channel), nullptr);
        
        ports.addChild (port, index, nullptr);
        ++index;
    }
    
    if (proc->acceptsMidi())
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Midi).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::input.toString(), nullptr)
            .setProperty (Slugs::name,  "MIDI", nullptr);
        ports.addChild (port, index, nullptr);
        index++;
    }
    
    if (proc->producesMidi())
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Midi).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::output.toString(), nullptr)
            .setProperty (Slugs::name,  "MIDI", nullptr);
        ports.addChild (port, index, nullptr);
        index++;
    }
}

void GraphNode::resetPorts()
{
    jassert(proc);
    
    ValueTree ports (metadata.getOrCreateChildWithName (Tags::ports, nullptr));
    metadata.removeChild (ports, nullptr);
    ports.removeAllChildren (nullptr);
    
    if (isAudioIONode() || isMidiIONode())
    {
        addPortsIONode (this, dynamic_cast<GraphProcessor::AudioGraphIOProcessor*>(proc.get()), ports);
        metadata.addChild (ports, 0, nullptr);
        return;
    }
    
    int index = 0;
    for (int channel = 0; channel < getNumAudioInputs(); ++channel)
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Audio).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::input.toString(), nullptr)
            .setProperty (Slugs::name,  proc->getInputChannelName (channel), nullptr);
        
        ports.addChild (port, index, nullptr);
        ++index;
    }
    
    for (int channel = 0; channel < getNumAudioOutputs(); ++channel)
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Audio).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::output.toString(), nullptr)
            .setProperty (Slugs::name,  proc->getOutputChannelName (channel), nullptr);
        
        ports.addChild (port, index, nullptr);
        ++index;
    }
    
    for (int i = 0; i < proc->getNumParameters(); ++i)
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Control).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::input.toString(), nullptr)
            .setProperty (Slugs::name,  "Param", nullptr);
        ports.addChild (port, index, nullptr);
        index++;
    }
    
    if (proc->acceptsMidi())
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Midi).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::input.toString(), nullptr)
            .setProperty (Slugs::name,  "MIDI Input", nullptr);
        ports.addChild (port, index, nullptr);
        index++;
    }
    
    if (proc->producesMidi())
    {
        ValueTree port (Tags::port);
        port.setProperty (Slugs::index, index, nullptr)
            .setProperty (Slugs::type,  PortType(PortType::Midi).getSlug(), nullptr)
            .setProperty (Tags::flow,   Tags::output.toString(), nullptr)
            .setProperty (Slugs::name,  "MIDI Output", nullptr);
        ports.addChild (port, index, nullptr);
        index++;
    }
    
    metadata.addChild (ports, 0, nullptr);
}

AudioPluginInstance* GraphNode::getAudioPluginInstance() const
{
    return dynamic_cast<AudioPluginInstance*> (proc.get());
}

GraphProcessor* GraphNode::getParentGraph() const
{
    return parent;
}

void GraphNode::setParentGraph (GraphProcessor* const graph)
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    parent = graph;
    if (IOP* const iop = dynamic_cast<IOP*> (proc.get()))
    {
        iop->setParentGraph (parent);
        metadata.setProperty (Slugs::name, iop->getName(), nullptr);
        resetPorts();
    }
}

}
