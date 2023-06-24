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
#include <element/audioengine.hpp>
#include <element/midipipe.hpp>
#include <element/processor.hpp>
#include "engine/rootgraph.hpp"
#include <element/node.hpp>

namespace element {

Processor::Processor (const PortList& portList)
    : nodeId (0),
      isPrepared (false),
      enablement (*this),
      midiProgramLoader (*this),
      portResetter (*this)
{
    parent = nullptr;
    gain.set (1.0f);
    lastGain.set (1.0f);
    inputGain.set (1.0f);
    lastInputGain.set (1.0f);
    oversampler = std::make_unique<Oversampler<float>>();
    // ports = portList;
    setPorts (portList);
}

Processor::Processor (const uint32 nodeId_) noexcept
    : nodeId (nodeId_),
      isPrepared (false),
      enablement (*this),
      midiProgramLoader (*this),
      portResetter (*this)
{
    parent = nullptr;
    gain.set (1.0f);
    lastGain.set (1.0f);
    inputGain.set (1.0f);
    lastInputGain.set (1.0f);
    oversampler = std::make_unique<Oversampler<float>>();
}

Processor::~Processor()
{
    clearParameters();
    enablement.cancelPendingUpdate();
    parent = nullptr;
}

void Processor::setRenderDetails (double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;
}

void Processor::clearParameters()
{
#if JUCE_DEBUG
    for (const auto* param : parameters)
        jassert (param->getReferenceCount() == 1);
#endif
    parameters.clear();
}

bool Processor::isSpecialParameter (int parameter)
{
    return parameter >= SpecialParameterBegin && parameter < SpecialParameterEnd;
}

String Processor::getSpecialParameterName (int parameter)
{
    String name = "N/A";

    switch (parameter)
    {
        case NoParameter:
            name = "None";
            break;
        case EnabledParameter:
            name = "Enable/Disable";
            break;
        case BypassParameter:
            name = "Bypass";
            break;
        case MuteParameter:
            name = "Mute";
            break;
    }

    return name;
}

const String& Processor::getTypeString() const
{
    return isA<GraphNode>() ? tags::graph.toString() : tags::plugin.toString();
}

bool Processor::containsParameter (const int index) const
{
    const auto numParams = getNumPorts (PortType::Control, true);
    return (index >= SpecialParameterBegin && index < SpecialParameterEnd) || (isPositiveAndBelow (index, numParams));
}

void Processor::setInputGain (const float f)
{
    inputGain.set (f);
}

void Processor::setGain (const float f)
{
    gain.set (f);
}

void Processor::getPluginDescription (PluginDescription& desc) const
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

void Processor::connectAudioTo (const Processor* other)
{
    jassert (getParentGraph());
    jassert (getParentGraph() == other->getParentGraph());

    auto& graph (*getParentGraph());
    AudioPluginInstance* const src = getAudioPluginInstance();
    AudioPluginInstance* const dst = other->getAudioPluginInstance();
    ignoreUnused (src, dst);
    const int totalChans = jmin (getNumAudioOutputs(), other->getNumAudioInputs());
    int failed = 0;

    for (int chan = 0; chan < totalChans; ++chan)
    {
        if (! graph.addConnection (
                this->nodeId, getPortForChannel (PortType::Audio, chan, false), other->nodeId, other->getPortForChannel (PortType::Audio, chan, true)))
        {
            ++failed;
        }
    }

    if (failed > 0)
    {
        DBG ("[element] failed connecting audio from " << src->getName() << " to " << dst->getName());
    }
}

bool Processor::isAudioInputNode() const
{
    if (auto* iop = dynamic_cast<const IONode*> (this))
        return iop->getType() == IONode::audioInputNode;
    return false;
}

bool Processor::isAudioOutputNode() const
{
    if (auto* iop = dynamic_cast<const IONode*> (this))
        return iop->getType() == IONode::audioOutputNode;
    return false;
}

bool Processor::isAudioIONode() const
{
    return isAudioInputNode() || isAudioOutputNode();
}

bool Processor::isMidiIONode() const
{
    if (auto* iop = dynamic_cast<const IONode*> (this))
        return iop->getType() == IONode::midiInputNode || iop->getType() == IONode::midiOutputNode;
    return false;
}

bool Processor::isMidiDeviceNode() const
{
    return nullptr != dynamic_cast<MidiDeviceProcessor*> (getAudioProcessor());
}

int Processor::getNumAudioInputs() const { return ports.size (PortType::Audio, true); }
int Processor::getNumAudioOutputs() const { return ports.size (PortType::Audio, false); }

void Processor::setInputRMS (int chan, float val)
{
    if (chan < inRMS.size())
        inRMS.getUnchecked (chan)->set (val);
}

void Processor::setOutputRMS (int chan, float val)
{
    if (chan < outRMS.size())
        outRMS.getUnchecked (chan)->set (val);
}

bool Processor::isSuspended() const
{
    return bypassed.get() == 1;
}

void Processor::suspendProcessing (const bool shouldBeSuspended)
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

bool Processor::isGraph() const noexcept { return isA<GraphNode>(); }
bool Processor::isRootGraph() const noexcept { return isA<RootGraph>(); }
bool Processor::isSubGraph() const noexcept { return isGraph() && ! isRootGraph(); }

PortType Processor::getPortType (const uint32 port) const
{
    const PortType t (ports.getType (static_cast<int> (port)));
    return t;
}

int Processor::getNumPorts (const PortType type, const bool isInput) const { return ports.size (type, isInput); }
uint32 Processor::getNumPorts() const { return (uint32) ports.size(); }

PortDescription Processor::getPort (int index) const
{
    auto port = ports.getPort (index);
    jassert (index == port.index);
    return port;
}

bool Processor::isPortInput (const uint32 port) const
{
    jassert (port < getNumPorts());
    return ports.isInput (static_cast<int> (port), false);
}

bool Processor::isPortOutput (const uint32 port) const
{
    jassert (port < getNumPorts());
    return ports.isOutput (static_cast<int> (port), true);
}

uint32 Processor::getPortForChannel (const PortType type, const int channel, const bool isInput) const
{
    return static_cast<uint32> (ports.getPortForChannel (type, channel, isInput));
}

int Processor::getChannelPort (const uint32 port) const
{
    return ports.getChannelForPort (static_cast<int> (port));
}

int Processor::getNthPort (const PortType type, const int index, bool isInput, bool oneBased) const
{
    int count = oneBased ? 0 : -1;

    jassert (getNumPorts() >= 0);
    uint32 nports = getNumPorts();

    for (uint32 port = 0; port < nports; ++port)
    {
        if (type == getPortType (port) && isInput == isPortInput (port))
        {
            if (++count == index)
            {
                return port;
            }
        }
    }

    jassertfalse;
    return EL_INVALID_PORT;
}

uint32 Processor::getMidiInputPort() const { return getPortForChannel (PortType::Midi, 0, true); }
uint32 Processor::getMidiOutputPort() const { return getPortForChannel (PortType::Midi, 0, false); }

void Processor::prepare (const double newSampleRate,
                         const int newBlockSize,
                         GraphNode* const parentGraph,
                         bool willBeEnabled)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;
    parent = parentGraph;

    if ((willBeEnabled || enabled.get() == 1) && ! isPrepared)
    {
        isPrepared = true;
        setParentGraph (parentGraph); //<< ensures io nodes get setup

        oversampler->prepare (jmax (getNumPorts (PortType::Audio, true),
                                    getNumPorts (PortType::Audio, false)),
                              blockSize);
        const int osFactor = jmax (1, getOversamplingFactor());
        prepareToRender (sampleRate * osFactor, blockSize * osFactor);

        inRMS.clearQuick (true);
        for (int i = 0; i < getNumAudioInputs(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set (0);
            inRMS.add (avf);
        }

        outRMS.clearQuick (true);
        for (int i = 0; i < getNumAudioOutputs(); ++i)
        {
            AtomicValue<float>* avf = new AtomicValue<float>();
            avf->set (0);
            outRMS.add (avf);
        }
    }
}

void Processor::unprepare()
{
    if (isPrepared)
    {
        isPrepared = false;
        releaseResources();
        oversampler->reset();
        inRMS.clear (true);
        outRMS.clear (true);
    }
}

void Processor::setEnabled (const bool shouldBeEnabled)
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

void Processor::EnablementUpdater::handleAsyncUpdate()
{
    graph.setEnabled (! graph.isEnabled());
}

//=============================================================================

void Processor::reloadMidiProgram()
{
    midiProgramLoader.triggerAsyncUpdate();
}

File Processor::getMidiProgramFile (int program) const
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
    stream << uids.toStdString() << "_" << std::setfill ('0') << std::setw (3) << program << ".eln";
    String fileName = stream.str();
    const File file (DataPath::defaultGlobalMidiProgramsDir().getChildFile (fileName));
    if (! file.getParentDirectory().exists())
        file.getParentDirectory().createDirectory();
    return file;
}

void Processor::saveMidiProgram()
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

void Processor::removeMidiProgram (int program, bool global)
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

Processor::MidiProgram* Processor::getMidiProgram (int program) const
{
    if (! isPositiveAndBelow (program, 128))
        return nullptr;
    for (auto* const p : midiPrograms)
        if (p->program == program)
            return p;
    auto* const ret = midiPrograms.add (new Processor::MidiProgram());
    ret->program = program;
    return ret;
}

void Processor::MidiProgramLoader::handleAsyncUpdate()
{
    const File programFile = node.getMidiProgramFile();
    const bool globalPrograms = node.useGlobalMidiPrograms();
    const auto requestedProgram = node.getMidiProgram();
#if 0
    if (node.lastMidiProgram.get() == requestedProgram)
    {
        DBG("[element] same program, not loading.");
        return;
    }
#endif

    if (globalPrograms)
    {
        if (programFile.existsAsFile())
        {
            const auto programData = Node::parse (programFile);
            auto data = programData.getProperty (tags::state).toString().trim();
            if (data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                {
                    node.lastMidiProgram.set (requestedProgram);
                    node.setState (state.getData(), (int) state.getSize());
                    DBG ("[element] loaded program: " << requestedProgram);
                }
            }
        }
        else
        {
            DBG ("[element] Program file doesn't exist: " << node.getMidiProgramFile().getFileName());
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
            DBG ("[element] program has no data");
        }
    }

    node.midiProgramChanged(); // always notify the program # changed even if not loaded.
        // do this because there may not be data for the program but
        // the property is still relavent.
}

void Processor::setMidiProgram (const int program)
{
    if (program < 0 || program > 127)
    {
        jassertfalse; // out of range
        return;
    }

    midiProgram.set (program);
}

void Processor::setMidiProgramName (const int program, const String& name)
{
    if (useGlobalMidiPrograms())
        return; // names not supported with global programs yet.

    if (auto* pr = getMidiProgram (program))
        pr->name = name;
}

String Processor::getMidiProgramName (const int program) const
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

void Processor::getMidiProgramsState (String& state) const
{
    state = String();
    if (midiPrograms.size() <= 0)
        return;
    ValueTree tree ("programs");
    for (auto* const program : midiPrograms)
    {
        auto& state = program->state;
        ValueTree data ("program");
        data.setProperty (tags::program, program->program, nullptr)
            .setProperty (tags::name, program->name, nullptr)
            .setProperty (tags::state, state.toBase64Encoding(), nullptr);
        tree.appendChild (data, nullptr);
    }

    MemoryOutputStream mo;
    {
        GZIPCompressorOutputStream gzipStream (mo, 9);
        tree.writeToStream (gzipStream);
    }

    state = mo.getMemoryBlock().toBase64Encoding();
}

void Processor::setMidiProgramsState (const String& state)
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
        std::unique_ptr<Processor::MidiProgram> program;
        program.reset (new Processor::MidiProgram());
        program->program = (int) data[tags::program];
        program->name = data[tags::name].toString();
        const auto state = data.getProperty (tags::state).toString().trim();
        if (state.isNotEmpty() && isPositiveAndBelow (program->program, 128))
        {
            program->state.fromBase64Encoding (state);
            midiPrograms.add (program.release());
        }
    }
}

//=============================================================================
void Processor::renderBypassed (AudioSampleBuffer& audio, MidiPipe& midi)
{
    audio.clear (0, audio.getNumSamples());
    midi.clear();
}

//=============================================================================
void Processor::setPorts (const PortList& newPorts)
{
    ports = newPorts;

    if (ports.size (PortType::Midi, true) <= 0 && ! isMidiIONode() && ! isAudioIONode() && ! isMidiDeviceNode())
    {
        ports.add (PortType::Midi, ports.size(), 0, "element_midi_input", "MIDI In", true);
    }

    ParameterArray newParams, newParamsOut;
    for (const auto* port : ports)
    {
        auto& parr = port->input ? newParams : newParamsOut;
        if (port->type == PortType::Control)
            parr.add (getOrCreateParameter (*port));
    }

    struct SortByArrayIndex
    {
        int compareElements (Parameter* lhs, Parameter* rhs)
        {
            return lhs->getParameterIndex() < rhs->getParameterIndex() ? -1 : 1;
        }
    } sorter;

    newParams.sort (sorter, true);
    parameters.swapWith (newParams);
    newParamsOut.sort (sorter, true);
    parametersOut.swapWith (newParamsOut);
}

ValueTree Processor::createPortsData() const
{
    ValueTree portList (tags::ports);

    for (int i = 0; i < ports.size(); ++i)
    {
        ValueTree port = ports.createValueTree (i);
        port.setProperty (tags::flow, ports.isInput (i) ? "input" : "output", nullptr);
        port.removeProperty (tags::input, nullptr); // added by KV modules, not needed yet
        portList.addChild (port, -1, 0);
        jassert (isPositiveAndBelow ((int) port.getProperty (tags::index), ports.size()));
    }

    return portList;
}

void Processor::resetPorts()
{
    refreshPorts();
}

GraphNode* Processor::getParentGraph() const { return parent; }

void Processor::setParentGraph (GraphNode* const graph)
{
    parent = graph;
    if (IONode* const iop = dynamic_cast<IONode*> (this))
    {
        iop->setParentGraph (parent);
        refreshPorts();
    }
}

void Processor::setMuted (bool muted)
{
    bool wasMuted = isMuted();
    mute.set (muted ? 1 : 0);
    if (wasMuted != isMuted())
        muteChanged (this);
}

dsp::Oversampling<float>* Processor::getOversamplingProcessor()
{
    return oversampler->getProcessor (osPow - 1);
}

void Processor::setOversamplingFactor (int osFactor)
{
    const auto newOsPow = (int) log2f ((float) osFactor);

    {
        ScopedLock sl (getPropertyLock());
        if (newOsPow == osPow)
            return;

        if (osFactor > 1)
        {
            osPow = (int) log2f ((float) osFactor);
            if (auto* const osProc = getOversamplingProcessor())
                osLatency = osProc->getLatencyInSamples();
        }
        else
        {
            osPow = 0;
            osLatency = 0.0;
        }
    }

    if (auto* g = getParentGraph())
        g->triggerAsyncUpdate();
}

int Processor::getOversamplingFactor()
{
    if (osPow > 0)
    {
        if (auto* osProc = getOversamplingProcessor())
            return static_cast<int> (osProc->getOversamplingFactor());
    }

    return 1;
}

//=========================================================================
void Processor::setDelayCompensation (double delayMs)
{
    if (delayCompMillis == delayMs)
        return;
    delayCompMillis = delayMs;
    jassert (sampleRate > 0.0);
    delayCompSamples = roundToInt (delayCompMillis * 0.001 * sampleRate);
}

double Processor::getDelayCompensation() const { return delayCompMillis; }
int Processor::getDelayCompensationSamples() const { return delayCompSamples; }

//=========================================================================
struct ChannelConnectionMap
{
    ChannelConnectionMap() {}
    ~ChannelConnectionMap() {}

    int channel;
    PortType type { PortType::Unknown };
    uint32 otherNodeId;
    uint32 otherNodePort;
};

void Processor::PortResetter::handleAsyncUpdate()
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

    node.refreshPorts();

    // Re-apply connections by channel
    for (const auto* ccs : sources)
    {
        graph->addConnection (ccs->otherNodeId, ccs->otherNodePort, node.nodeId, node.getPortForChannel (ccs->type, ccs->channel, true));
    }

    for (const auto* dss : destinations)
    {
        graph->addConnection (node.nodeId, node.getPortForChannel (dss->type, dss->channel, false), dss->otherNodeId, dss->otherNodePort);
    }
    graph->removeIllegalConnections();

    // notify others
    node.portsChanged();
}

void Processor::triggerPortReset()
{
    portResetter.cancelPendingUpdate();
    portResetter.triggerAsyncUpdate();
}

//=========================================================================
int Processor::getLatencySamples() const
{
    return latencySamples + delayCompSamples + roundToInt (osLatency);
}

void Processor::setLatencySamples (int latency)
{
    if (latency == latencySamples)
        return;
    latencySamples = latency;
}

//=========================================================================
ParameterPtr Processor::getOrCreateParameter (const PortDescription& port)
{
    jassert (port.type == PortType::Control);
    if (port.type != PortType::Control)
        return nullptr;

    auto param = getParameter (port);

    if (param == nullptr)
    {
        param = new RangedParameter (port);
    }

    if (param != nullptr)
    {
        param->parameterIndex = port.channel;
    }

    jassert (param != nullptr);
    return param;
}

} // namespace element
