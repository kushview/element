#include "ElementApp.h"
#include "engine/GraphNode.h"
#include "engine/GraphProcessor.h"
#include "engine/PluginWrapper.h"
#include "session/Node.h"

namespace Element {
    
static void setNodePropertiesFrom (const PluginDescription& pd, ValueTree& p)
{
    p.setProperty (Slugs::name, pd.name, nullptr);
    if (pd.descriptiveName != pd.name)
        p.setProperty("descriptiveName", pd.descriptiveName, nullptr);
    
    p.setProperty ("format",       pd.pluginFormatName, nullptr);
    p.setProperty ("category",     pd.category, nullptr);
    p.setProperty ("manufacturer", pd.manufacturerName, nullptr);
    p.setProperty ("version",      pd.version, nullptr);
    p.setProperty ("file",         pd.fileOrIdentifier, nullptr);
    p.setProperty ("uid",          String::toHexString (pd.uid), nullptr);
    p.setProperty ("isInstrument", pd.isInstrument, nullptr);
    p.setProperty ("fileTime",     String::toHexString (pd.lastFileModTime.toMilliseconds()), nullptr);
    p.setProperty ("numInputs",    pd.numInputChannels, nullptr);
    p.setProperty ("numOutputs",   pd.numOutputChannels, nullptr);
    p.setProperty ("isShell",      pd.hasSharedContainer, nullptr);
    // p.setProperty ("isSuspended",  plugin->isSuspended(), nullptr);
}

GraphNode::GraphNode (const uint32 nodeId_, Processor* const processor_) noexcept
    : nodeId (nodeId_),
      proc (processor_),
      isPrepared (false),
      metadata (Tags::node)
{
    parent = nullptr;
    gain.set(1.0f); lastGain.set(1.0f);
    inputGain.set(1.0f); lastInputGain.set(1.0f);
    jassert (proc != nullptr);
    PluginDescription desc;
    getPluginDescription (desc);
    setNodePropertiesFrom (desc, metadata);
    metadata.setProperty (Slugs::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Slugs::name, proc->getName(), nullptr)
            .setProperty (Slugs::type, "plugin", nullptr)
            .setProperty ("numAudioIns", getNumAudioInputs(), nullptr)
            .setProperty ("numAudioOuts", getNumAudioOutputs(), nullptr);
    
    ValueTree ports (metadata.getOrCreateChildWithName ("ports", nullptr));
    auto* inst = getAudioPluginInstance();
    for (uint32 p = 0; p < (uint32) Processor::getNumPorts(inst); ++p)
    {
        ValueTree port (Tags::port);
        const PortType type (proc->getPortType (p));
        const bool isInput (proc->isPortInput(p));
        
        if (type != PortType::Audio)
            continue;
        
        port.setProperty (Slugs::index, (int)p, nullptr)
            .setProperty (Slugs::type, type.getSlug(), nullptr)
            .setProperty (Tags::flow, isInput ? Tags::input.toString() : Tags::output.toString(), nullptr);
        
        const int channel = proc->getChannelPort (p);
        port.setProperty (Slugs::name, isInput ? inst->getInputChannelName (channel)
                                               : inst->getOutputChannelName (channel),
                          nullptr);
        
        ports.addChild (port, -1, nullptr);
    }
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

int GraphNode::getNumAudioInputs() const
{
    if (AudioPluginInstance* inst = getAudioPluginInstance())
        return inst->getTotalNumInputChannels();
    return 0;
}

int GraphNode::getNumAudioOutputs() const
{
    if (AudioPluginInstance* inst = getAudioPluginInstance())
        return inst->getTotalNumOutputChannels();
    return 0;
}

void GraphNode::setInputRMS (int chan, float val)
{
    if (chan < inRMS.size()) {
        inRMS.getUnchecked(chan)->set(val);
    }
}

void GraphNode::setOutputRMS (int chan, float val)
{
    if (chan < outRMS.size()) {
        outRMS.getUnchecked(chan)->set(val);
    }
}

bool GraphNode::isSuspended() const
{
    if (AudioPluginInstance* inst = getAudioPluginInstance())
        inst->isSuspended();
    return true;
}

void GraphNode::suspendProcessing (const bool shouldBeSuspended)
{
    if (AudioPluginInstance* inst = getAudioPluginInstance())
        inst->suspendProcessing(shouldBeSuspended);
}

uint32 GraphNode::getMidiInputPort() const
{
    return proc->getNthPort (PortType::Atom, 0, true, false);
}

uint32 GraphNode::getMidiOutputPort() const
{
    return proc->getNthPort (PortType::Atom, 0, false, false);
}

bool GraphNode::isSubgraph() const noexcept
{
    return (dynamic_cast<GraphProcessor*> (proc.get()) != nullptr);
}

void GraphNode::prepare (const double sampleRate, const int blockSize,
                         GraphProcessor* const graph)
{
    parent = graph;
    if (! isPrepared)
    {
        AudioPluginInstance* instance = getAudioPluginInstance();
        instance->setPlayConfigDetails (instance->getTotalNumInputChannels(),
                                        instance->getTotalNumOutputChannels(),
                                        sampleRate, blockSize);
        setParentGraph (graph);
        instance->prepareToPlay (sampleRate, blockSize);
        
        if (nullptr != dynamic_cast<IOProcessor*> (instance)) {
            resetPorts();
        }
        
        inRMS.clearQuick(true);
        for (int i = 0; i < instance->getTotalNumInputChannels(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set(0);
            inRMS.add (avf);
        }

        outRMS.clearQuick(true);
        for (int i = 0; i < instance->getTotalNumOutputChannels(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set(0);
            outRMS.add(avf);
        }

        isPrepared = true;
    }
}

void GraphNode::unprepare()
{
    if (isPrepared)
    {
        isPrepared = false;
        inRMS.clear(true);
        outRMS.clear(true);
        proc->releaseResources();
        parent = nullptr;
    }
}

    void GraphNode::resetPorts()
    {
        ValueTree ports (metadata.getOrCreateChildWithName ("ports", nullptr));
        metadata.removeChild (ports, nullptr);
        ports = ValueTree (Tags::ports);
        
        auto* inst = getAudioPluginInstance();
//        const uint32 numPorts = (uint32) Processor::getNumPorts (inst);
//        const int numIns = parent->getTotalNumInputChannels(); // inst->getTotalNumInputChannels();
//        const int numOuts = parent->getTotalNumOutputChannels(); // inst->getTotalNumOutputChannels();
        
        for (uint32 p = 0; p < (uint32) Processor::getNumPorts (inst); ++p)
        {
            ValueTree port (Tags::port);
            const PortType type (proc->getPortType (p));
            const bool isInput (proc->isPortInput(p));
            
            if (type != PortType::Audio)
                continue;
            
            port.setProperty (Slugs::index, (int)p, nullptr)
                .setProperty (Slugs::type, type.getSlug(), nullptr)
                .setProperty (Tags::flow, isInput ? Tags::input.toString() : Tags::output.toString(), nullptr);
            
            const int channel = proc->getChannelPort (p);
            port.setProperty (Slugs::name, isInput ? inst->getInputChannelName (channel)
                                                   : inst->getOutputChannelName (channel),
                              nullptr);
            
            ports.addChild (port, -1, nullptr);
        }
        
        metadata.addChild (ports, 0, nullptr);
    }
    
AudioPluginInstance* GraphNode::getAudioPluginInstance() const
{
    if (PluginWrapper* wrapper = dynamic_cast<PluginWrapper*> (proc.get()))
        return wrapper->getWrappedAudioPluginInstance();

    return dynamic_cast<AudioPluginInstance*> (proc.get());
}

GraphProcessor* GraphNode::getParentGraph() const
{
    return parent;
}

void GraphNode::setParentGraph (GraphProcessor* const graph)
{
    parent = graph;
    if (GraphProcessor::AudioGraphIOProcessor* const ioProc
            = dynamic_cast<GraphProcessor::AudioGraphIOProcessor*> (proc.get()))
        ioProc->setParentGraph (graph);
#if 0
    // not yet supported
    else if (GraphPort* const ioProc = dynamic_cast <GraphPort*> (proc.get()))
        ioProc->setGraph (graph);
#endif
}

}
