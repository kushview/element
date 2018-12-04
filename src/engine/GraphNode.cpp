#include "ElementApp.h"
#include "engine/AudioEngine.h"
#include "engine/GraphNode.h"
#include "engine/GraphProcessor.h"
#include "engine/PlaceholderProcessor.h"
#include "engine/SubGraphProcessor.h"
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
      metadata (Tags::node),
      enablement (*this)
{
    parent = nullptr;
    proc = processor_;
    gain.set(1.0f); lastGain.set (1.0f);
    inputGain.set(1.0f); lastInputGain.set (1.0f);
    jassert (proc != nullptr);
    
    PluginDescription desc;
    getPluginDescription (desc);
    
    setNodePropertiesFrom (desc, metadata);
    metadata.setProperty (Slugs::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Slugs::name, proc->getName(), nullptr)
            .setProperty (Slugs::type, getTypeString(), nullptr)
            .setProperty ("numAudioIns", getNumAudioInputs(), nullptr)
            .setProperty ("numAudioOuts", getNumAudioOutputs(), nullptr);
    resetPorts();
}

GraphNode::~GraphNode()
{
    enablement.cancelPendingUpdate();
    pluginState.reset();
    parent = nullptr;
    proc = nullptr;
}

const String& GraphNode::getTypeString() const
{ 
    return (nullptr == dynamic_cast<GraphProcessor*> (proc.get()))
        ? Tags::plugin.toString() : Tags::graph.toString();
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
    int failed = 0;

    for (int chan = 0; chan < totalChans; ++chan)
    {
        if (! graph.addConnection (
            this->nodeId, getPortForChannel (PortType::Audio, chan, false),
            other->nodeId, other->getPortForChannel (PortType::Audio, chan, true)))
        {
            ++failed;
        }
    }

    if (failed > 0)
    {
        DBG("[EL] failed connecting audio from " << src->getName() << " to " << dst->getName());
    }
}

bool GraphNode::isAudioIONode() const
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    if (IOP* iop = processor<IOP>())
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

bool GraphNode::isGraph() const noexcept { return (nullptr != dynamic_cast<GraphProcessor*> (proc.get())); }
bool GraphNode::isSubGraph() const noexcept { return (nullptr != dynamic_cast<SubGraphProcessor*> (proc.get())); }
bool GraphNode::isRootGraph() const noexcept { return (nullptr != dynamic_cast<RootGraph*> (proc.get())); }

PortType GraphNode::getPortType (const uint32 port) const
{
    const PortType t (ports.getType (static_cast<int> (port)));
    return t;
}

int GraphNode::getNumPorts (const PortType type, const bool isInput) const { return ports.size (type, isInput); }
uint32 GraphNode::getNumPorts() const { return (uint32) ports.size(); }
bool GraphNode::isPortInput (const uint32 port)  const { return ports.isInput (port, false); }
bool GraphNode::isPortOutput (const uint32 port) const { return ports.isOutput (port, true); }

uint32 GraphNode::getPortForChannel (const PortType type, const int channel, const bool isInput) const
{
    return static_cast<uint32> (ports.getPortForChannel (type, channel, isInput));
}

int GraphNode::getChannelPort (const uint32 port) const
{
    return ports.getChannelForPort (static_cast<int> (port));
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
    return getPortForChannel (PortType::Midi, 0, false);
}

void GraphNode::prepare (const double sampleRate, const int blockSize,
                         GraphProcessor* const parentGraph,
                         bool willBeEnabled)
{
    parent = parentGraph;
    AudioProcessor* instance = proc.get();
    
    if ((willBeEnabled || enabled.get() == 1) && !isPrepared)
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
    }
}

void GraphNode::setEnabled (const bool shouldBeEnabled)
{
    if (shouldBeEnabled == isEnabled())
        return;

    if (! MessageManager::getInstance()->isThisTheMessageThread())
    {
        enablement.cancelPendingUpdate();
        enablement.triggerAsyncUpdate();
        return;
    }

    if (shouldBeEnabled)
    {
        if (parent)
        {
            prepare (parent->getSampleRate(), parent->getBlockSize(), parent, true);
            if (pluginState.getSize() > 0)
            {
                proc->setStateInformation (pluginState.getData(), (int) pluginState.getSize());
                pluginState.reset();
                jassert (pluginState.getSize() == 0);
            }
            enabled.set (1);
        }
        else
        {
            enabled.set (0);
        }
    }
    else
    {
        enabled.set (0);

        if (proc)
        {
            if (pluginState.getSize() > 0)
                pluginState.reset();
            proc->getStateInformation (pluginState);
        }

        unprepare();
    }

    enablementChanged (this);
}

void GraphNode::EnablementUpdater::handleAsyncUpdate()
{
    DBG("[EL] Async node enabled changed");
    graph.setEnabled (! graph.isEnabled());
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
    jassert (proc);
    createPorts(); // TODO: should be a standalone operation

    ValueTree portList (metadata.getOrCreateChildWithName (Tags::ports, nullptr));
    ValueTree nodeList (metadata.getOrCreateChildWithName (Tags::nodes, nullptr));
    metadata.removeChild (portList, nullptr);
    metadata.removeChild (nodeList, nullptr);
    portList.removeAllChildren (nullptr);

    for (int i = 0; i < ports.size(); ++i)
    {
        ValueTree port = ports.createValueTree (i);
        port.setProperty (Tags::flow, ports.isInput(i) ? "input" : "output", nullptr);
        port.removeProperty (Tags::input, nullptr); // added by KV modules, not needed yet
        portList.addChild (port, -1, 0);
    }

    metadata.addChild (nodeList, 0, nullptr);
    metadata.addChild (portList, 1, nullptr);
    jassert (metadata.getChildWithName(Tags::ports).getNumChildren() == ports.size());

    if (auto* sub = dynamic_cast<SubGraphProcessor*> (proc.get()))
        for (int i = 0; i < sub->getNumNodes(); ++i)
            sub->getNode(i)->resetPorts();
}

void GraphNode::createPorts()
{
    kv::PortList newPorts;

    int index = 0;

    for (int channel = 0; channel < getNumAudioInputs(); ++channel)
    {
        String symbol = "audio_in_"; symbol << channel;
        newPorts.add (PortType::Audio, index, channel, symbol,
                      proc->getInputChannelName (channel), true);
        ++index;
    }
    
    for (int channel = 0; channel < getNumAudioOutputs(); ++channel)
    {
        String symbol = "audio_out_"; symbol << channel;
        newPorts.add (PortType::Audio, index, channel, symbol,
                      proc->getOutputChannelName (channel), false);
        ++index;
    }
    
    for (int i = 0; i < proc->getParameters().size(); ++i)
    {
        String symbol = "control_"; symbol << i;
        newPorts.add (PortType::Control, index, i, symbol,
                      proc->getParameterName(i), true);
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
