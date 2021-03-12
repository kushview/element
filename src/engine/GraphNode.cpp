/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <iomanip>
#include "ElementApp.h"

#include "engine/nodes/AudioProcessorNode.h"
#include "engine/nodes/MidiDeviceProcessor.h"
#include "engine/nodes/PlaceholderProcessor.h"
#include "engine/nodes/SubGraphProcessor.h"

#include "engine/AudioEngine.h"
#include "engine/GraphNode.h"
#include "engine/GraphProcessor.h"
#include "engine/MidiPipe.h"

#include "session/Node.h"

namespace Element {

GraphNode::GraphNode (const uint32 nodeId_) noexcept
    : nodeId (nodeId_),
      metadata (Tags::node),
      isPrepared (false),
      enablement (*this),
      midiProgramLoader (*this),
      portResetter (*this)
{
    parent = nullptr;
    gain.set(1.0f); lastGain.set (1.0f);
    inputGain.set(1.0f); lastInputGain.set (1.0f);
    metadata.setProperty (Slugs::id, static_cast<int64> (nodeId), nullptr)
            .setProperty (Slugs::type, getTypeString(), nullptr);
}

GraphNode::~GraphNode()
{
    clearParameters();
    enablement.cancelPendingUpdate();
    parent = nullptr;
}

void GraphNode::clearParameters()
{
   #if JUCE_DEBUG
    for (const auto* param : parameters)
        jassert(param->getReferenceCount() == 1);
   #endif
    parameters.clear();
}

bool GraphNode::isSpecialParameter (int parameter)
{
    return parameter >= SpecialParameterBegin && parameter < SpecialParameterEnd;
}

String GraphNode::getSpecialParameterName (int parameter)
{
    String name = "N/A";

    switch (parameter)
    {
        case NoParameter:       name = "None"; break;
        case EnabledParameter:  name = "Enable/Disable"; break;
        case BypassParameter:   name = "Bypass"; break;
        case MuteParameter:     name = "Mute"; break;
    }

    return name;
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

void GraphNode::getPluginDescription (PluginDescription& desc) const
{
    if (AudioPluginInstance* i = getAudioPluginInstance())
    {
        i->fillInPluginDescription (desc);
    }
    else
    {
        // need to fill this in for custom nodes
        jassertfalse;
    }
}

void GraphNode::connectAudioTo (const GraphNode* other)
{
    jassert (getParentGraph());
    jassert (getParentGraph() == other->getParentGraph());

    GraphProcessor& graph (*getParentGraph());
    AudioPluginInstance* const src = getAudioPluginInstance();
    AudioPluginInstance* const dst = other->getAudioPluginInstance();
    ignoreUnused (src, dst);
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

bool GraphNode::isMidiDeviceNode() const
{
    return nullptr != dynamic_cast<MidiDeviceProcessor*> (getAudioProcessor());
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

PortDescription GraphNode::getPort (int index) const
{
    auto port = ports.getPort (index);
    jassert (index == port.index);
    return port;
}

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

void GraphNode::prepare (const double newSampleRate, const int blockSize,
                         GraphProcessor* const parentGraph,
                         bool willBeEnabled)
{
    sampleRate = newSampleRate;
    parent = parentGraph;

    if ((willBeEnabled || enabled.get() == 1) && !isPrepared)
    {
        isPrepared = true;
        setParentGraph (parentGraph); //<< ensures io nodes get setup

        initOversampling (jmax (getNumPorts (PortType::Audio, true), 
                                getNumPorts (PortType::Audio, false)), 
                                blockSize);

        const int osFactor = getOversamplingFactor();
        prepareToRender (sampleRate * osFactor, blockSize * osFactor);

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
        releaseResources();
        resetOversampling();
        inRMS.clear (true);
        outRMS.clear (true);
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

//=============================================================================

void GraphNode::reloadMidiProgram()
{
    midiProgramLoader.triggerAsyncUpdate();
}

File GraphNode::getMidiProgramFile (int program) const
{
    PluginDescription desc;
    getPluginDescription (desc);
    const auto uids = desc.createIdentifierString();
    if (! isPositiveAndBelow (program, 128))
        program = getMidiProgram();

    if (uids.isEmpty())
    {
        jassertfalse;
        return File();
    }

    if (! isPositiveAndBelow (program, 128))
    {
        jassertfalse;
        return File();
    }

    std::stringstream stream;
    stream << uids.toStdString() << "_" << std::setfill('0') << std::setw(3) << program << ".eln";
    String fileName = stream.str();
    const File file (DataPath::applicationDataDir().getChildFile("NodeMidiPrograms").getChildFile(fileName));
    if (! file.getParentDirectory().exists())
        file.getParentDirectory().createDirectory();
    return file;
}

void GraphNode::saveMidiProgram()
{
    if (useGlobalMidiPrograms())
        return; // don't save global programs here.
    
    int progamNumber = midiProgram.get();
    if (! isPositiveAndBelow (progamNumber, 128))
        return;
    if (auto* const program = getMidiProgram (progamNumber))
    {
        program->state = MemoryBlock();
        getState (program->state);
    }
}

void GraphNode::removeMidiProgram (int program, bool global)
{
    if (! isPositiveAndBelow (program, 128))
        return;
    
    if (global)
    {
        const auto file = getMidiProgramFile (program);
        if (file.existsAsFile())
            file.deleteFile();
    }
    else
    {
        for (int i = midiPrograms.size(); --i >= 0;)
        {
            auto* const progobj = midiPrograms.getUnchecked (i);
            if (progobj->program == program)
                midiPrograms.remove (i);
        }
    }
}

GraphNode::MidiProgram* GraphNode::getMidiProgram (int program) const
{
    if (! isPositiveAndBelow (program, 128))
        return nullptr;
    for (auto* const p : midiPrograms)
        if (p->program == program)
            return p;
    auto* const ret = midiPrograms.add (new GraphNode::MidiProgram ());
    ret->program = program;
    return ret;
}

void GraphNode::MidiProgramLoader::handleAsyncUpdate()
{
    const File programFile = node.getMidiProgramFile();
    const bool globalPrograms = node.useGlobalMidiPrograms();
    const auto requestedProgram = node.getMidiProgram();
   #if 0
    if (node.lastMidiProgram.get() == requestedProgram)
    {
        DBG("[EL] same program, not loading.");
        return;
    }
   #endif

    if (globalPrograms)
    {
        if (programFile.existsAsFile())
        {
            const auto programData = Node::parse (programFile);
            auto data = programData.getProperty(Tags::state).toString().trim();
            if (data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                {
                    node.lastMidiProgram.set (requestedProgram);
                    node.setState (state.getData(), (int) state.getSize());
                    DBG("[EL] loaded program: " << requestedProgram);
                }
            }
        }
        else
        {
            DBG("[EL] Program file doesn't exist: " << node.getMidiProgramFile().getFileName());
        }
    }
    else
    {
        if (auto* const program = node.getMidiProgram (requestedProgram))
        {
            node.setState (program->state.getData(), 
                           static_cast<int> (program->state.getSize()));
        }
        else
        {
            DBG("[EL] program has no data");
        }
    }

    node.midiProgramChanged(); // always notify the program # changed even if not loaded.
                               // do this because there may not be data for the program but
                               // the property is still relavent.
}

void GraphNode::setMidiProgram (const int program)
{
    if (program < 0 || program > 127)
    {
        jassertfalse; // out of range
        return;
    }
    
    midiProgram.set (program);
}

void GraphNode::setMidiProgramName (const int program, const String& name) 
{
    if (useGlobalMidiPrograms())
        return; // names not supported with global programs yet.

    if (auto* pr = getMidiProgram (program))
        pr->name = name;
}

String GraphNode::getMidiProgramName (const int program) const
{
    if (useGlobalMidiPrograms())
    {
        String name ("Global ");
        name << (program + 1);
        return name;
    }

    if (auto* pr = getMidiProgram (program))
        return pr->name;
    return {};
}

void GraphNode::getMidiProgramsState (String& state) const
{
    state = String();
    if (midiPrograms.size() <= 0)
        return;
    ValueTree tree ("programs");
    for (auto* const program : midiPrograms)
    {
        auto& state = program->state;
        ValueTree data ("program");
        data.setProperty (Tags::program, program->program, nullptr)
            .setProperty (Tags::name, program->name, nullptr)
            .setProperty (Tags::state, state.toBase64Encoding(), nullptr);
        tree.appendChild (data, nullptr);
    }

    MemoryOutputStream mo;
    {
        GZIPCompressorOutputStream gzipStream (mo, 9);
        tree.writeToStream (gzipStream);
    }

    state = mo.getMemoryBlock().toBase64Encoding();
}

void GraphNode::setMidiProgramsState (const String& state)
{
    midiPrograms.clearQuick (true);
    if (state.isEmpty())
        return;
    MemoryBlock mb;
    mb.fromBase64Encoding (state);
    const ValueTree tree = (mb.getSize() > 0)
        ? ValueTree::readFromGZIPData (mb.getData(), mb.getSize())
        : ValueTree();
    
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        const auto data = tree.getChild (i);
        std::unique_ptr<GraphNode::MidiProgram> program;
        program.reset (new GraphNode::MidiProgram());
        program->program = (int) data [Tags::program];
        program->name = data[Tags::name].toString();
        const auto state = data.getProperty (Tags::state).toString().trim();
        if (state.isNotEmpty() && isPositiveAndBelow (program->program, 128))
        {
            program->state.fromBase64Encoding (state);
            midiPrograms.add (program.release());
        }
    }
}

//=============================================================================

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
    
    if (ports.size (PortType::Midi, true) <= 0 &&
        !isMidiIONode() && !isAudioIONode() && !isMidiDeviceNode())
    {
        ports.add (PortType::Midi, ports.size(), 0, "element_midi_input", "MIDI In", true);
    }

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
    
    parameters.clear();
    for (int i = 0; i < ports.size(); ++i)
    {
        const auto port = ports.getPort (i);
        if (port.input && port.type == PortType::Control)
            parameters.add (getOrCreateParameter (port));
    }
    
    struct ParamSorter
    {
        int compareElements (Parameter* lhs, Parameter* rhs)
        {
            return lhs->getParameterIndex() < rhs->getParameterIndex() ? -1 : 1;
        }
    } sorter;
    parameters.sort (sorter, true);

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

void GraphNode::setMuted (bool muted)
{
    bool wasMuted = isMuted();
    mute.set (muted ? 1 : 0);
    if (wasMuted != isMuted())
        muteChanged (this);
}

void GraphNode::initOversampling (int numChannels, int blockSize)
{
    osProcessors.clear();
    numChannels = jmax (1, numChannels); // avoid assertion on nodes that don't have audio
    for (int p = 1; p <= maxOsPow; ++p)
        osProcessors.add (new dsp::Oversampling<float> (numChannels, 
            p, dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR));

    prepareOversampling (blockSize);
}

void GraphNode::prepareOversampling (int blockSize)
{
    for (auto* osProcessor : osProcessors)
        osProcessor->initProcessing (blockSize);
}

void GraphNode::resetOversampling()
{
    for (auto* osProcessor : osProcessors)
        osProcessor->reset();
}

dsp::Oversampling<float>* GraphNode::getOversamplingProcessor()
{
    return osProcessors[osPow-1];
}

void GraphNode::setOversamplingFactor (int osFactor)
{
    osPow = (int) log2f ((float) osFactor);
    if (auto* osProc = getOversamplingProcessor())
        osLatency = osProc->getLatencyInSamples();
}

int GraphNode::getOversamplingFactor()
{
    if (osPow > 0)
        if (auto* osProc = getOversamplingProcessor())
            return static_cast<int> (osProc->getOversamplingFactor());

    return 1;
}

//=========================================================================
void GraphNode::setDelayCompensation (double delayMs)
{
    if (delayCompMillis == delayMs)
        return;
    delayCompMillis = delayMs;
    jassert (sampleRate > 0.0);
    delayCompSamples = roundToInt (delayCompMillis * 0.001 * sampleRate);
}

double GraphNode::getDelayCompensation()        const { return delayCompMillis; }
int GraphNode::getDelayCompensationSamples()    const { return delayCompSamples; }

//=========================================================================
struct ChannelConnectionMap
{
    ChannelConnectionMap() { }
    ~ChannelConnectionMap() { }

    int channel;
    PortType type { PortType::Unknown };
    uint32 otherNodeId;
    uint32 otherNodePort;
};

void GraphNode::PortResetter::handleAsyncUpdate()
{
    auto* const graph = node.getParentGraph();
    jassert (graph != nullptr);

    // Cache existing connections by channel.
    OwnedArray<ChannelConnectionMap> sources, destinations;
    for (int i = graph->getNumConnections(); --i >= 0;)
    {
        const auto* const c = graph->getConnection (i);
        if (c->destNode == node.nodeId)
        {
            auto* m = sources.add (new ChannelConnectionMap());
            m->type = node.getPortType (c->destPort);
            m->channel = node.getChannelPort (c->destPort);
            m->otherNodeId = c->sourceNode;
            m->otherNodePort = c->sourcePort;
        }
        else if (c->sourceNode == node.nodeId)
        {
            auto* d = destinations.add (new ChannelConnectionMap());
            d->type = node.getPortType (c->sourcePort);
            d->channel = node.getChannelPort (c->sourcePort);
            d->otherNodeId = c->destNode;
            d->otherNodePort = c->destPort;
        }
    }

    node.resetPorts();

    // Re-apply connections by channel
    for (const auto* ccs : sources) {
        graph->addConnection (ccs->otherNodeId, ccs->otherNodePort, node.nodeId,
                              node.getPortForChannel (ccs->type, ccs->channel, true));
    }

    for (const auto* dss : destinations) {
        graph->addConnection (node.nodeId, node.getPortForChannel (dss->type, dss->channel, false),
                              dss->otherNodeId, dss->otherNodePort);
    }
    graph->removeIllegalConnections();

    // notify others
    node.portsChanged();
}

void GraphNode::triggerPortReset()
{
    portResetter.cancelPendingUpdate();
    portResetter.triggerAsyncUpdate();
}

//=========================================================================
int GraphNode::getLatencySamples() const
{
    return latencySamples + delayCompSamples + roundFloatToInt (osLatency);
}

void GraphNode::setLatencySamples (int latency)
{
    if (latency == latencySamples)
        return;
    latencySamples = latency;
}

//=========================================================================
Parameter::Ptr GraphNode::getOrCreateParameter (const PortDescription& port)
{
    jassert (port.type == PortType::Control && port.input == true);
    if (port.type != PortType::Control && port.input != true)
        return nullptr;
    
    auto param = getParameter (port);
    
    if (param == nullptr)
    {
        param = new ControlPortParameter (port);
    }

    if (param != nullptr)
    {
        param->parameterIndex = port.channel;
    }

    jassert(param != nullptr);
    return param;
}

}
