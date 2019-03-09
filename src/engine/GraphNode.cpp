#include "ElementApp.h"

#include "engine/nodes/AudioProcessorNode.h"
#include "engine/AudioEngine.h"
#include "engine/GraphNode.h"
#include "engine/GraphProcessor.h"
#include "engine/MidiPipe.h"
#include "engine/nodes/PlaceholderProcessor.h"
#include "engine/nodes/SubGraphProcessor.h"

#include "session/Node.h"

namespace Element {

GraphNode::GraphNode (const uint32 nodeId_) noexcept
    : nodeId (nodeId_),
      isPrepared (false),
      metadata (Tags::node),
      enablement (*this)
{
    parent = nullptr;
    gain.set(1.0f); lastGain.set (1.0f);
    inputGain.set(1.0f); lastInputGain.set (1.0f);
    metadata.setProperty (Slugs::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Slugs::type, getTypeString(), nullptr);
    // resetPorts();
}

GraphNode::~GraphNode()
{
    enablement.cancelPendingUpdate();
    parent = nullptr;
}

const String& GraphNode::getTypeString() const
{ 
    return (nullptr == processor<GraphProcessor>())
        ? Tags::plugin.toString() : Tags::graph.toString();
}

bool GraphNode::containsParameter (const int index) const
{
    const auto numParams = getNumPorts (PortType::Control, true);
    return (index >= SpecialParameterBegin && index < SpecialParameterEnd) ||
        (isPositiveAndBelow (index, numParams));
}

GraphNode* GraphNode::createForRoot (GraphProcessor* g)
{
    auto* node = new AudioProcessorNode (0, g);
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

bool GraphNode::isAudioInputNode() const
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    if (IOP* iop = processor<IOP>())
        return iop->getType() == IOP::audioInputNode;
    return false;
}

bool GraphNode::isAudioOutputNode() const
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    if (IOP* iop = processor<IOP>())
        return iop->getType() == IOP::audioOutputNode;
    return false;
}

bool GraphNode::isAudioIONode() const
{
    return isAudioInputNode() || isAudioOutputNode();
}

bool GraphNode::isMidiIONode() const
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    if (IOP* iop = dynamic_cast<IOP*> (getAudioProcessor()))
        return iop->getType() == IOP::midiInputNode || iop->getType() == IOP::midiOutputNode;
    return false;
}

int GraphNode::getNumAudioInputs()      const { return ports.size (PortType::Audio, true); }
int GraphNode::getNumAudioOutputs()     const { return ports.size (PortType::Audio, false); }

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

bool GraphNode::isSuspended() const
{
    return bypassed.get() == 1;
}

void GraphNode::suspendProcessing (const bool shouldBeSuspended)
{
    const bool wasSuspeneded = isSuspended();
    const int iShouldBeSuspended = static_cast<int> (shouldBeSuspended);

    if (auto* proc = getAudioProcessor())
    {
        if (wasSuspeneded != shouldBeSuspended)
        {
            proc->suspendProcessing (shouldBeSuspended);
            bypassed.set (proc->isSuspended() ? 1 : 0);
        }
    }
    else if (bypassed.get() != iShouldBeSuspended)
    {
        bypassed.set (iShouldBeSuspended);
    }

    if (isSuspended() != wasSuspeneded)
        bypassChanged (this);
}

bool GraphNode::isGraph() const noexcept        { return nullptr != dynamic_cast<GraphProcessor*> (getAudioProcessor()); }
bool GraphNode::isSubGraph() const noexcept     { return nullptr != dynamic_cast<SubGraphProcessor*> (getAudioProcessor()); }
bool GraphNode::isRootGraph() const noexcept    { return nullptr != dynamic_cast<RootGraph*> (getAudioProcessor()); }

PortType GraphNode::getPortType (const uint32 port) const
{
    const PortType t (ports.getType (static_cast<int> (port)));
    return t;
}

int GraphNode::getNumPorts (const PortType type, const bool isInput) const { return ports.size (type, isInput); }
uint32 GraphNode::getNumPorts() const { return (uint32) ports.size(); }

bool GraphNode::isPortInput (const uint32 port)  const 
{
    jassert (port < getNumPorts());
    return ports.isInput (static_cast<int> (port), false); 
}

bool GraphNode::isPortOutput (const uint32 port) const
{
    jassert (port < getNumPorts());
    return ports.isOutput (static_cast<int> (port), true); 
}

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

uint32 GraphNode::getMidiInputPort()  const { return getPortForChannel (PortType::Midi, 0, true); }
uint32 GraphNode::getMidiOutputPort() const { return getPortForChannel (PortType::Midi, 0, false); }

void GraphNode::prepare (const double sampleRate, const int blockSize,
                         GraphProcessor* const parentGraph,
                         bool willBeEnabled)
{
    parent = parentGraph;
    if ((willBeEnabled || enabled.get() == 1) && !isPrepared)
    {
        isPrepared = true;
        setParentGraph (parentGraph); //<< ensures io nodes get setup
        prepareToRender (sampleRate, blockSize);

        // TODO: move model code out of engine code
        // VERIFY: this portion is actually needed. This was here to ensure
        // port information is available before setting up the RMS buffers
        if (! isAudioIONode() && ! isMidiIONode())
            resetPorts();

        // VERIFY: this is needed.  GraphManager should be setting this
        if (metadata.getProperty (Tags::bypass, false))
            suspendProcessing (true);

        inRMS.clearQuick (true);
        for (int i = 0; i < getNumAudioInputs(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set(0);
            inRMS.add (avf);
        }

        outRMS.clearQuick (true);
        for (int i = 0; i < getNumAudioOutputs(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set(0);
            outRMS.add(avf);
        }
    }
}

void GraphNode::unprepare()
{
    if (isPrepared)
    {
        isPrepared = false;
        inRMS.clear (true);
        outRMS.clear (true);
        releaseResources();
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
        unprepare();
    }

    enablementChanged (this);
}

void GraphNode::EnablementUpdater::handleAsyncUpdate()
{
    graph.setEnabled (! graph.isEnabled());
}

void GraphNode::renderBypassed (AudioSampleBuffer& audio, MidiPipe& midi)
{
    audio.clear (0, audio.getNumSamples());
    midi.clear();
}

void GraphNode::resetPorts()
{
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
        
        if (auto* root = dynamic_cast<RootGraph*> (getParentGraph()))
        {
            if (isAudioInputNode() && ports.getType(i) == PortType::Audio && ports.isOutput (i))
            {
                port.setProperty (Tags::name, root->getInputChannelName (ports.getChannelForPort (i)), nullptr);
            }
            else if (isAudioOutputNode() && ports.getType(i) == PortType::Audio && ports.isInput (i))
            {
                port.setProperty (Tags::name, root->getOutputChannelName (ports.getChannelForPort (i)), nullptr);
            }
        }

        portList.addChild (port, -1, 0);
        jassert (isPositiveAndBelow ((int)port.getProperty(Tags::index), ports.size()));
    }

    metadata.addChild (nodeList, 0, nullptr);
    metadata.addChild (portList, 1, nullptr);
    jassert (metadata.getChildWithName(Tags::ports).getNumChildren() == ports.size());

    if (auto* sub = dynamic_cast<SubGraphProcessor*> (getAudioProcessor()))
        for (int i = 0; i < sub->getNumNodes(); ++i)
            sub->getNode(i)->resetPorts();
}

GraphProcessor* GraphNode::getParentGraph() const { return parent; }

void GraphNode::setParentGraph (GraphProcessor* const graph)
{
    typedef GraphProcessor::AudioGraphIOProcessor IOP;
    parent = graph;
    if (IOP* const iop = dynamic_cast<IOP*> (getAudioProcessor()))
    {
        iop->setParentGraph (parent);
        metadata.setProperty (Slugs::name, iop->getName(), nullptr);
        resetPorts();
    }
}

}
