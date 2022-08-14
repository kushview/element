/*
    This file is part of Element
    Copyright (C) 2014-2019  Kushview, LLC.  All rights reserved.

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

#include "engine/nodes/BaseProcessor.h" // for internal id macros
#include "session/node.hpp"
#include "session/session.hpp"
#include "controllers/GraphManager.h"
#include "scopedflag.hpp"

namespace Element {

//==============================================================================
struct NameSorter
{
    NameSorter() {}
    int compareElements (const Node& lhs, const Node& rhs)
    {
        return lhs.getName() < rhs.getName() ? -1 : lhs.getName() == rhs.getName() ? 0 : 1;
    }
    friend class NodeArray;
};

static void readPluginDescriptionForLoading (const ValueTree& p, PluginDescription& pd)
{
    const auto& type = p.getProperty (Tags::type);

    if (type == "graph")
    {
        pd.name = p.getProperty (Tags::name);
        pd.fileOrIdentifier = "element.graph";
        pd.pluginFormatName = "Element";
    }
    else
    {
        // plugins and io nodes
        pd.name = p.getProperty (Tags::pluginName);
        pd.pluginFormatName = p.getProperty (Tags::format);
        pd.fileOrIdentifier = p.getProperty (Tags::identifier);
        if (pd.fileOrIdentifier.isEmpty())
            pd.fileOrIdentifier = p.getProperty (Tags::file);
    }
}

static ValueTree getBlockValueTree (const Node& node)
{
    return node.getUIValueTree().getOrCreateChildWithName ("block", nullptr);
}

static StringArray getHiddenPortsProperty (const Node& node)
{
    const auto bv = getBlockValueTree (node);
    auto result = StringArray::fromTokens (bv.getProperty ("hiddenPorts").toString(), ",", "\"\'");
    result.trim();
    return result;
}

static void setHiddenPortsProperty (const Node& node, const StringArray& symbols)
{
    auto bv = getBlockValueTree (node);
    bv.setProperty ("hiddenPorts", symbols.joinIntoString (","), nullptr);
}

//==============================================================================
int Port::getChannel() const
{
    const Node node (objectData.getParent().getParent());
    if (auto* g = node.getObject())
        return g->getChannelPort (getIndex());
    return -1;
}

Node Port::getNode() const
{
    return Node (getNodeValueTree(), false);
}

void Port::setHiddenOnBlock (bool hidden)
{
    // if (hidden == isHiddenOnBlock())
    //     return;

    const auto node (getNode());
    auto symbols = getHiddenPortsProperty (node);

    if (hidden)
    {
        symbols.addIfNotAlreadyThere (getSymbol());
    }
    else
    {
        symbols.removeString (getSymbol().toRawUTF8());
    }

    setHiddenPortsProperty (node, symbols);
}

bool Port::isHiddenOnBlock() const
{
    const auto parent (getNode());
    if (! parent.isValid())
    {
        return true;
    }

    const auto symbols = getHiddenPortsProperty (parent);
    return symbols.contains (getSymbol().toRawUTF8());
}

//==============================================================================
Node::Node() : ObjectModel() {}

Node::Node (const ValueTree& data, const bool setMissing)
    : ObjectModel (data)
{
    if (setMissing)
    {
        jassert (data.hasType (Tags::node));
        setMissingProperties();
    }
}

Node::Node (const Identifier& nodeType)
    : ObjectModel (Tags::node)
{
    objectData.setProperty (Slugs::type, nodeType.toString(), nullptr);
    setMissingProperties();
}

Node::~Node() noexcept {}

//=============================================================================
Node Node::createDefaultGraph (const String& name)
{
    Node graph (Tags::graph);
    graph.setProperty (Tags::name, name);
    ValueTree nodes = graph.getNodesValueTree();

    const auto types = StringArray ({ "audio.input", "audio.output", "midi.input", "midi.output" });
    const auto names = StringArray ({ "Audio In", "Audio Out", "MIDI In", "MIDI Out" });
    uint32 nodeId = 1;

    for (const auto& t : types)
    {
        ValueTree ioNode (Tags::node);
        ValueTree ports = ioNode.getOrCreateChildWithName (Tags::ports, 0);
        int portIdx = 0;

        ioNode.setProperty (Tags::id, static_cast<int64> (nodeId++), 0)
            .setProperty (Tags::type, "plugin", 0)
            .setProperty (Tags::format, "Internal", 0)
            .setProperty (Tags::identifier, t, 0)
            .setProperty (Tags::name, names[types.indexOf (t)], 0);

        if (t == "audio.input")
        {
            ioNode.setProperty (Tags::relativeX, 0.25f, 0)
                .setProperty (Tags::relativeY, 0.25f, 0)
                .setProperty ("numAudioIns", 0, 0)
                .setProperty ("numAudioOuts", 2, 0);

            ValueTree port (Tags::port);
            port.setProperty ("name", "Port", 0)
                .setProperty ("index", portIdx++, 0)
                .setProperty ("type", "audio", 0)
                .setProperty ("flow", "output", 0);
            ports.addChild (port, -1, 0);

            port = ValueTree (Tags::port);
            port.setProperty ("name", "Port", 0)
                .setProperty ("index", portIdx++, 0)
                .setProperty ("type", "audio", 0)
                .setProperty ("flow", "output", 0);
            ports.addChild (port, -1, 0);
        }
        else if (t == "audio.output")
        {
            ioNode.setProperty (Tags::relativeX, 0.25f, 0)
                .setProperty (Tags::relativeY, 0.75f, 0)
                .setProperty ("numAudioIns", 2, 0)
                .setProperty ("numAudioOuts", 0, 0);

            ValueTree port (Tags::port);
            port.setProperty ("name", "Port", 0)
                .setProperty ("index", portIdx++, 0)
                .setProperty ("type", "audio", 0)
                .setProperty ("flow", "input", 0);
            ports.addChild (port, -1, 0);

            port = ValueTree (Tags::port);
            port.setProperty ("name", "Port", 0)
                .setProperty ("index", portIdx++, 0)
                .setProperty ("type", "audio", 0)
                .setProperty ("flow", "input", 0);
            ports.addChild (port, -1, 0);
        }
        else if (t == "midi.input")
        {
            ioNode.setProperty (Tags::relativeX, 0.75f, 0)
                .setProperty (Tags::relativeY, 0.25f, 0)
                .setProperty ("numAudioIns", 0, 0)
                .setProperty ("numAudioOuts", 0, 0);
            ValueTree port (Tags::port);
            port.setProperty ("name", "Port", 0)
                .setProperty ("index", portIdx++, 0)
                .setProperty ("type", "midi", 0)
                .setProperty ("flow", "output", 0);
            ports.addChild (port, -1, 0);
        }
        else if (t == "midi.output")
        {
            ioNode.setProperty (Tags::relativeX, 0.75f, 0)
                .setProperty (Tags::relativeY, 0.75f, 0)
                .setProperty ("numAudioIns", 0, 0)
                .setProperty ("numAudioOuts", 0, 0);
            ValueTree port (Tags::port);
            port.setProperty ("name", "Port", 0)
                .setProperty ("index", portIdx++, 0)
                .setProperty ("type", "midi", 0)
                .setProperty ("flow", "input", 0);
            ports.addChild (port, -1, 0);
        }

        Node finalNode (ioNode, true);
        nodes.addChild (finalNode.getValueTree(), -1, 0);
    }

    return graph;
}

const String Node::getPluginName() const
{
    if (NodeObjectPtr object = getObject())
        return object->getName();
    return {};
}

bool Node::isProbablyGraphNode (const ValueTree& data)
{
    return data.hasType (Tags::node) && Tags::graph.toString() == data.getProperty (Tags::type).toString();
}

ValueTree Node::resetIds (const ValueTree& data)
{
    ValueTree result = data;
    jassert (result.hasType (Tags::node)); // must be a node
    jassert (! result.getParent().isValid()); // cannot be part of another object tree
    if (result.getParent().isValid())
        return result;
    result.removeProperty (Tags::id, nullptr);
    result.setProperty (Tags::uuid, Uuid().toString(), nullptr);
    return result;
}

ValueTree Node::parse (const File& file)
{
    ValueTree sessionData = Session::readFromFile (file);
    if (sessionData.isValid())
    {
        const auto graphs = sessionData.getChildWithName (Tags::graphs);
        const auto sessionNode = graphs.getChild (graphs.getProperty (Tags::active, 0));
        return sessionNode.createCopy();
    }

    ValueTree data;
    ValueTree nodeData;

    if (auto e = XmlDocument::parse (file))
    {
        data = ValueTree::fromXml (*e);
    }
    else
    {
        FileInputStream input (file);
        data = ValueTree::readFromStream (input);
    }

    if (data.hasType (Tags::node))
    {
        nodeData = data;
    }
    else
    {
        nodeData = data.getChildWithName (Tags::node);
        // Rename the node appropriately
        if (data.hasProperty (Tags::name))
            nodeData.setProperty (Tags::name, data.getProperty (Tags::name), 0);
        else
            nodeData.setProperty (Tags::name, file.getFileNameWithoutExtension(), 0);
    }

    if (nodeData.isValid() && nodeData.hasType (Tags::node))
    {
        if (data.indexOf (nodeData) >= 0)
            data.removeChild (nodeData, 0);

        Node::sanitizeProperties (nodeData);
        return nodeData;
    }

    return ValueTree();
}

void Node::sanitizeProperties (ValueTree node, const bool recursive)
{
    node.removeProperty (Tags::updater, nullptr);
    node.removeProperty (Tags::object, nullptr);

    if (node.hasType (Tags::node))
    {
        Array<Identifier> properties ({ Tags::offline,
                                        Tags::placeholder,
                                        Tags::missing });
        for (const auto& property : properties)
            node.removeProperty (property, nullptr);
    }

    if (recursive)
        for (int i = 0; i < node.getNumChildren(); ++i)
            sanitizeProperties (node.getChild (i), recursive);
}

void Node::sanitizeRuntimeProperties (ValueTree node, const bool recursive)
{
    Node::sanitizeProperties (node, recursive);
}

bool Node::writeToFile (const File& targetFile) const
{
    ValueTree data = objectData.createCopy();
    sanitizeProperties (data, true);

#if EL_SAVE_BINARY_FORMAT
    TemporaryFile tempFile (targetFile);
    if (auto out = std::unique_ptr<FileOutputStream> (tempFile.getFile().createOutputStream()))
    {
        data.writeToStream (*out);
        out.reset();
        return tempFile.overwriteTargetFileWithTemporary();
    }
#else
    if (auto e = data.createXml())
        return e->writeToFile (targetFile, String());
#endif

    return false;
}

bool Node::savePresetTo (const DataPath& path, const String& name) const
{
    {
        // hack: ensure the plugin's state info is up-to-date
        Node (*this).savePluginState();
    }

    ValueTree preset (Tags::preset);
    ValueTree data = objectData.createCopy();
    sanitizeProperties (data, true);
    preset.addChild (data, -1, 0);

    const auto targetFile = path.createNewPresetFile (*this, name);
    data.setProperty (Tags::name, targetFile.getFileNameWithoutExtension(), 0);
    data.setProperty (Tags::type, Tags::node.toString(), 0);

#if EL_SAVE_BINARY_FORMAT
    TemporaryFile tempFile (targetFile);
    if (auto out = std::unique_ptr<FileOutputStream> (tempFile.getFile().createOutputStream()))
    {
        data.writeToStream (*out);
        out.reset();
        return tempFile.overwriteTargetFileWithTemporary();
    }
#else
    if (auto e = preset.createXml())
        return e->writeToFile (targetFile, String());
#endif
    return false;
}

Node Node::createGraph (const String& name)
{
    Node node (Tags::graph);
    ValueTree root = node.getValueTree();
    root.setProperty (Tags::name, name, nullptr);
    root.getOrCreateChildWithName (Tags::nodes, nullptr);
    root.getOrCreateChildWithName (Tags::arcs, nullptr);
    return node;
}

ValueTree Node::makeArc (const Arc& arc)
{
    ValueTree model (Tags::arc);
    model.setProperty (Tags::sourceNode, (int) arc.sourceNode, nullptr);
    model.setProperty (Tags::sourcePort, (int) arc.sourcePort, nullptr);
    model.setProperty (Tags::destNode, (int) arc.destNode, nullptr);
    model.setProperty (Tags::destPort, (int) arc.destPort, nullptr);
    return model;
}

const bool Node::canConnectTo (const Node& o) const
{
    if (objectData.getParent() != o.objectData.getParent() || objectData == o.objectData)
    {
        return false;
    }

    return true;
}

ValueTree Node::getParentArcsNode() const
{
    ValueTree arcs = objectData.getParent();
    if (arcs.hasType (Tags::nodes))
        arcs = arcs.getParent();
    if (! arcs.isValid())
        return ValueTree();

    jassert (arcs.hasType (Tags::node));
    return arcs.getOrCreateChildWithName (Tags::arcs, nullptr);
}

const String Node::getDisplayName() const
{
    String name = getName();
    if (name.isEmpty())
        name = getPluginName();
    return name;
}

void Node::getPluginDescription (PluginDescription& p) const
{
    readPluginDescriptionForLoading (objectData, p);
}

void Node::setMissingProperties()
{
    stabilizePropertyString (Tags::uuid, Uuid().toString());
    stabilizePropertyString (Tags::type, "default");
    stabilizePropertyString (Tags::name, "Node");
    stabilizeProperty (Tags::bypass, false);
    stabilizeProperty (Tags::persistent, true);
    stabilizePropertyString (Tags::renderMode, "single");
    stabilizeProperty (Tags::keyStart, 0);
    stabilizeProperty (Tags::keyEnd, 127);
    stabilizeProperty (Tags::transpose, 0);
    stabilizeProperty (Tags::delayCompensation, 0);
    stabilizeProperty (Tags::tempo, (double) 120.0);
    objectData.getOrCreateChildWithName (Tags::nodes, nullptr);
    objectData.getOrCreateChildWithName (Tags::ports, nullptr);
    auto ui = objectData.getOrCreateChildWithName (Tags::ui, nullptr);
    ui.getOrCreateChildWithName (Tags::block, nullptr);
}

NodeObject* Node::getObject() const
{
    return dynamic_cast<NodeObject*> (objectData.getProperty (Tags::object, var()).getObject());
}

NodeObject* Node::getObjectForId (const uint32 nodeId) const
{
    const Node node (getNodeById (nodeId));
    return node.isValid() ? node.getObject() : nullptr;
}

void Node::getPorts (PortArray& ports, PortType type, bool isInput) const
{
    const ValueTree portList (getPortsValueTree());
    for (int i = 0; i < portList.getNumChildren(); ++i)
    {
        const Port port (portList.getChild (i));
        if (port.isA (type, isInput))
            ports.add (port);
    }
}

void Node::getPorts (PortArray& ins, PortArray& outs, PortType type) const
{
    const ValueTree portList (getPortsValueTree());
    for (int i = 0; i < portList.getNumChildren(); ++i)
    {
        const Port port (portList.getChild (i));
        if (port.isA (type, true))
            ins.add (port);
        else if (port.isA (type, false))
            outs.add (port);
    }
}

void Node::getAudioInputs (PortArray& ports) const
{
    getPorts (ports, PortType::Audio, true);
}

void Node::getAudioOutputs (PortArray& ports) const
{
    getPorts (ports, PortType::Audio, false);
}

//==============================================================================
bool Node::isMidiInputDevice() const
{
    return objectData.getProperty (Tags::format) == "Element" && objectData.getProperty (Tags::identifier) == EL_INTERNAL_ID_MIDI_INPUT_DEVICE;
}

bool Node::isMidiOutputDevice() const
{
    return objectData.getProperty (Tags::format) == "Element" && objectData.getProperty (Tags::identifier) == EL_INTERNAL_ID_MIDI_OUTPUT_DEVICE;
}

//==============================================================================
void Node::resetPorts()
{
    if (NodeObjectPtr ptr = getObject())
    {
        ptr->refreshPorts();
        ValueTree newPorts = ptr->createPortsData();
        ValueTree ports = getPortsValueTree();
        objectData.removeChild (ports, nullptr);
        objectData.addChild (newPorts, -1, nullptr);
    }
}

void Node::getPossibleSources (NodeArray& a) const
{
    ValueTree nodes = objectData.getParent();
    if (! nodes.hasType (Tags::nodes))
        return;

    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        const Node child (nodes.getChild (i));
        if (child.getNodeId() == getNodeId())
            continue;
        if (child.canConnectTo (*this))
            a.add (child);
    }
}

void Node::getPossibleDestinations (NodeArray& a) const
{
    ValueTree nodes = objectData.getParent();
    if (! nodes.hasType (Tags::nodes))
        return;
    for (int i = 0; i < nodes.getNumChildren(); ++i)
    {
        const Node child (nodes.getChild (i));
        if (child.getNodeId() == getNodeId())
            continue;
        if (canConnectTo (child))
            a.add (child);
    }
}

Arc Node::arcFromValueTree (const ValueTree& data)
{
    Arc arc ((uint32) (int) data.getProperty (Tags::sourceNode, (int) KV_INVALID_NODE),
             (uint32) (int) data.getProperty (Tags::sourcePort, (int) KV_INVALID_PORT),
             (uint32) (int) data.getProperty (Tags::destNode, (int) KV_INVALID_NODE),
             (uint32) (int) data.getProperty (Tags::destPort, (int) KV_INVALID_PORT));
    return arc;
}

int Node::getNumConnections() const { return getArcsValueTree().getNumChildren(); }
ValueTree Node::getConnectionValueTree (const int index) const { return getArcsValueTree().getChild (index); }

void NodeArray::sortByName()
{
    NameSorter sorter;
    this->sort (sorter);
}

bool Node::connectionExists (const ValueTree& arcs,
                             const uint32 sourceNode,
                             const uint32 sourcePort,
                             const uint32 destNode,
                             const uint32 destPort,
                             const bool checkMissing)
{
    for (int i = arcs.getNumChildren(); --i >= 0;)
    {
        const ValueTree arc (arcs.getChild (i));
        if (static_cast<int> (sourceNode) == (int) arc.getProperty (Tags::sourceNode) && static_cast<int> (sourcePort) == (int) arc.getProperty (Tags::sourcePort) && static_cast<int> (destNode) == (int) arc.getProperty (Tags::destNode) && static_cast<int> (destPort) == (int) arc.getProperty (Tags::destPort))
        {
            return (checkMissing) ? ! arc.getProperty (Tags::missing, false) : true;
        }
    }

    return false;
}

Node Node::getNodeById (const uint32 nodeId) const
{
    const ValueTree nodes = getNodesValueTree();
    Node node (nodes.getChildWithProperty (Tags::id, static_cast<int64> (nodeId)), false);
    return node;
}

static Node findNodeRecursive (const Node& node, const Uuid& uuid)
{
    Node found;
    for (int i = node.getNumNodes(); --i >= 0;)
    {
        found = node.getNode (i);
        if (found.getUuid() == uuid)
            return found;
        found = findNodeRecursive (found, uuid);
        if (found.isValid())
            break;
    }
    return found;
}

Node Node::getNodeByUuid (const Uuid& uuid, const bool recursive) const
{
    if (! recursive)
    {
        const ValueTree nodes = getNodesValueTree();
        Node node (nodes.getChildWithProperty (Tags::uuid, uuid.toString()), false);
        return node;
    }

    return findNodeRecursive (*this, uuid);
}

Port Node::getPort (const int index) const
{
    Port port (getPortsValueTree().getChildWithProperty (Tags::index, index));
    return port;
}

bool Node::canConnect (const uint32 sourceNode, const uint32 sourcePort, const uint32 destNode, const uint32 destPort) const
{
    const Node sn (getNodeById (sourceNode));
    const Node dn (getNodeById (destNode));
    if (! sn.isValid() || ! dn.isValid())
        return false;

    const Port dp (dn.getPort ((int) destPort));
    const Port sp (sn.getPort ((int) sourcePort));
    return sp.getType().canConnect (dp.getType());
}

void Node::setRelativePosition (const double x, const double y)
{
    setProperty (Tags::relativeX, x);
    setProperty (Tags::relativeY, y);
}

void Node::getRelativePosition (double& x, double& y) const
{
    x = (double) getProperty (Tags::relativeX, 0.5f);
    y = (double) getProperty (Tags::relativeY, 0.5f);
}

bool Node::hasPosition() const
{
    return hasProperty (Tags::x) && hasProperty (Tags::y);
}

void Node::getPosition (double& x, double& y) const
{
    x = (double) getProperty (Tags::x, 0.0);
    y = (double) getProperty (Tags::y, 0.0);
}

void Node::setPosition (double x, double y)
{
    setProperty (Tags::x, x);
    setProperty (Tags::y, y);
}

Node Node::getParentGraph() const
{
    ValueTree parent = objectData.getParent();

    while (! isProbablyGraphNode (parent))
    {
        if (! parent.isValid())
            break;
        parent = parent.getParent();
    }

    return isProbablyGraphNode (parent) ? Node (parent, false)
                                        : Node();
}

bool Node::descendsFrom (const Node& graph) const
{
    auto parent = getParentGraph();
    while (graph.isValid() && parent.isValid())
    {
        if (graph == parent)
            return true;
        parent = parent.getParentGraph();
    }
    return false;
}

bool Node::isChildOfRootGraph() const
{
    const auto graph (getParentGraph());
    return graph.isRootGraph();
}

kv::MidiChannels Node::getMidiChannels() const
{
    kv::MidiChannels chans;
    if (objectData.hasProperty (Tags::midiChannels))
    {
        if (auto* const block = objectData.getProperty (Tags::midiChannels).getBinaryData())
        {
            BigInteger data;
            data.loadFromMemoryBlock (*block);
            chans.setChannels (data);
        }
    }
    else
    {
        const auto channel = (int) objectData.getProperty (Tags::midiChannel, 0);
        if (channel > 0)
            chans.setChannel (channel);
        else
            chans.setOmni (true);
    }
    return chans;
}

void Node::restorePluginState()
{
    if (! isValid())
        return;

    if (NodeObjectPtr obj = getObject())
    {
        if (auto* const proc = obj->getAudioProcessor())
        {
            const int wantedProgram = objectData.getProperty (Tags::program, -1);
            const bool shouldSetProgram = proc->getNumPrograms() > 0 && isPositiveAndBelow (wantedProgram, proc->getNumPrograms());
            if (shouldSetProgram)
                proc->setCurrentProgram (wantedProgram);

            auto data = getProperty (Tags::state).toString().trim();
            if (data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                {
                    proc->setStateInformation (state.getData(), (int) state.getSize());
                }
            }

            data = getProperty (Tags::programState).toString().trim();
            if (shouldSetProgram && data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                {
                    proc->setCurrentProgramStateInformation (state.getData(),
                                                             (int) state.getSize());
                }
            }
        }
        else
        {
            const int wantedProgram = objectData.getProperty (Tags::program, -1);
            const bool shouldSetProgram = obj->getNumPrograms() > 0 && isPositiveAndBelow (wantedProgram, obj->getNumPrograms());
            if (shouldSetProgram)
                obj->setCurrentProgram (wantedProgram);

            auto data = getProperty (Tags::state).toString().trim();
            if (data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                    obj->setState (state.getData(), (int) state.getSize());
            }
        }

        if (hasProperty (Tags::bypass))
        {
            obj->suspendProcessing (isBypassed());
        }

        if (hasProperty (Tags::gain))
        {
            obj->setGain (getProperty ("gain"));
        }

        if (hasProperty ("inputGain"))
        {
            obj->setInputGain (getProperty ("inputGain"));
        }

        if (hasProperty (Tags::keyStart) && hasProperty (Tags::keyEnd))
        {
            Range<int> range (getProperty (Tags::keyStart, 0),
                              getProperty (Tags::keyEnd, 127));
            obj->setKeyRange (range);
        }

        if (hasProperty (Tags::midiChannels))
        {
            const MidiChannels channels (getMidiChannels());
            obj->setMidiChannels (channels.get());
        }

        if (hasProperty (Tags::midiProgram))
        {
            obj->setMidiProgram ((int) getProperty (Tags::midiProgram, -1));
        }

        if (hasProperty (Tags::midiProgramsEnabled))
            obj->setMidiProgramsEnabled ((bool) getProperty (Tags::midiProgramsEnabled, true));
        obj->setUseGlobalMidiPrograms ((bool) getProperty (Tags::globalMidiPrograms, obj->useGlobalMidiPrograms()));
        if (hasProperty (Tags::midiProgramsState))
            obj->setMidiProgramsState (getProperty (Tags::midiProgramsState).toString().trim());

        obj->setMuted ((bool) getProperty (Tags::mute, obj->isMuted()));
        obj->setMuteInput ((bool) getProperty ("muteInput", obj->isMutingInputs()));

        if (hasProperty (Tags::transpose))
            obj->setTransposeOffset (getProperty (Tags::transpose));

        obj->setOversamplingFactor (jmax (1, (int) getProperty (Tags::oversamplingFactor, 1)));
        obj->setDelayCompensation (getProperty (Tags::delayCompensation, 0.0));
    }

    // this was originally here to help reduce memory usage
    // need another way to free this property without disturbing
    // the normal flow of the app.
    const bool clearStateProperty = false;
    if (clearStateProperty)
        objectData.removeProperty (Tags::state, 0);

    for (int i = 0; i < getNumNodes(); ++i)
        getNode (i).restorePluginState();
}

void Node::savePluginState()
{
    if (! isValid())
        return;

    NodeObjectPtr obj = getObject();
    if (obj && obj->isPrepared)
    {
        MemoryBlock state;

        if (auto* proc = obj->getAudioProcessor())
        {
            proc->getStateInformation (state);
            if (state.getSize() > 0)
            {
                objectData.setProperty (Tags::state, state.toBase64Encoding(), nullptr);
            }
            else
            {
                const bool clearStateProperty = false;
                if (clearStateProperty)
                    objectData.removeProperty (Tags::state, 0);
            }

            state.reset();
            proc->getCurrentProgramStateInformation (state);
            if (state.getSize() > 0)
            {
                objectData.setProperty (Tags::programState, state.toBase64Encoding(), 0);
            }

            setProperty (Tags::bypass, proc->isSuspended());
            setProperty (Tags::program, proc->getCurrentProgram());
        }
        else
        {
            obj->getState (state);
            if (state.getSize() > 0)
                objectData.setProperty (Tags::state, state.toBase64Encoding(), nullptr);
        }

        setProperty (Tags::midiProgram, obj->getMidiProgram());
        setProperty (Tags::globalMidiPrograms, obj->useGlobalMidiPrograms());
        setProperty (Tags::midiProgramsEnabled, obj->areMidiProgramsEnabled());
        setProperty (Tags::mute, obj->isMuted());
        setProperty ("muteInput", obj->isMutingInputs());
        String mps;
        obj->getMidiProgramsState (mps);
        setProperty (Tags::midiProgramsState, mps);
        setProperty (Tags::oversamplingFactor, obj->getOversamplingFactor());
        setProperty (Tags::delayCompensation, obj->getDelayCompensation());
    }

    for (int i = 0; i < getNumNodes(); ++i)
        getNode (i).savePluginState();
}

void Node::setMuted (bool shouldBeMuted)
{
    if (shouldBeMuted != isMuted())
        setProperty (Tags::mute, shouldBeMuted);
    if (auto* obj = getObject())
        obj->setMuted (isMuted());
}

void Node::setMuteInput (bool shouldMuteInputs)
{
    if (shouldMuteInputs != isMutingInputs())
        setProperty ("muteInput", shouldMuteInputs);
    if (auto* obj = getObject())
        obj->setMuteInput (isMutingInputs());
}

void Node::setCurrentProgram (const int index)
{
    if (auto* obj = getObject())
        obj->setCurrentProgram (index);
}

int Node::getCurrentProgram() const
{
    if (auto* obj = getObject())
        return obj->getCurrentProgram();
    return -1;
}

String Node::getProgramName (const int index) const
{
    if (auto* obj = getObject())
        return obj->getProgramName (index);
    return String();
}

int Node::getNumPrograms() const
{
    if (auto* obj = getObject())
        return obj->getNumPrograms();
    return 0;
}

bool Node::isA (const String& format, const String& identifier) const
{
    return getFormat().toString() == format && getIdentifier().toString() == identifier;
}

bool Node::hasEditor() const
{
    if (Tags::plugin == getNodeType())
        if (auto gn = getObject())
            if (auto* const proc = gn->getAudioProcessor())
                return proc->hasEditor();
    return false;
}

void ConnectionBuilder::addConnections (GraphManager& controller, const uint32 targetNodeId) const
{
    NodeObjectPtr tgt = controller.getNodeForId (targetNodeId);
    if (tgt)
    {
        bool anythingAdded = false;
        for (const auto* pc : portChannelMap)
        {
            NodeObjectPtr ptr = controller.getNodeForId (pc->nodeId);
            if (! ptr)
                continue;

            if (pc->isInput)
            {
                anythingAdded |= controller.addConnection (
                    tgt->nodeId, tgt->getPortForChannel (pc->type, pc->targetChannel, ! pc->isInput), ptr->nodeId, ptr->getPortForChannel (pc->type, pc->nodeChannel, pc->isInput));
            }
            else
            {
                anythingAdded |= controller.addConnection (
                    ptr->nodeId, ptr->getPortForChannel (pc->type, pc->nodeChannel, pc->isInput), tgt->nodeId, tgt->getPortForChannel (pc->type, pc->targetChannel, ! pc->isInput));
            }
        }

        if (anythingAdded)
            controller.syncArcsModel();
    }
    else
    {
        lastError = "Could not find target node";
    }
}

void Node::getArcs (OwnedArray<Arc>& results) const
{
    const ValueTree arcs (getParentArcsNode());
    for (int i = 0; i < arcs.getNumChildren(); ++i)
    {
        std::unique_ptr<Arc> arc;
        arc.reset (new Arc (arcFromValueTree (arcs.getChild (i))));
        if (arc->sourceNode == getNodeId() || arc->destNode == getNodeId())
            results.add (arc.release());
    }
}

void Node::forEach (std::function<void (const ValueTree& tree)> handler) const
{
    forEach (objectData, handler);
}

void Node::forEach (const ValueTree tree, std::function<void (const ValueTree& tree)> handler) const
{
    handler (tree);
    for (int i = 0; i < tree.getNumChildren(); ++i)
        forEach (tree.getChild (i), handler);
}

// default value here must match that as defined in NodeObject.h
bool Node::useGlobalMidiPrograms() const { return (bool) getProperty (Tags::globalMidiPrograms, false); }
void Node::setUseGlobalMidiPrograms (bool useGlobal)
{
    if (NodeObjectPtr obj = getObject())
    {
        if (obj->useGlobalMidiPrograms() == useGlobal)
            return;
        obj->setUseGlobalMidiPrograms (useGlobal);
        setProperty (Tags::globalMidiPrograms, obj->useGlobalMidiPrograms());
    }
}

// default value here must match that as defined in NodeObject.h
bool Node::areMidiProgramsEnabled() const { return (bool) getProperty (Tags::midiProgramsEnabled, false); }
void Node::setMidiProgramsEnabled (bool useMidiPrograms)
{
    if (NodeObjectPtr obj = getObject())
    {
        if (obj->areMidiProgramsEnabled() == useMidiPrograms)
            return;
        obj->setMidiProgramsEnabled (useMidiPrograms);
        setProperty (Tags::midiProgramsEnabled, obj->areMidiProgramsEnabled());
    }
}

int Node::getMidiProgram() const { return (int) getProperty (Tags::midiProgram, 0); }
void Node::setMidiProgram (int program)
{
    if (NodeObjectPtr obj = getObject())
    {
        if (obj->getMidiProgram() == program)
            return;
        obj->setMidiProgram (program);
        setProperty (Tags::midiProgram, obj->areMidiProgramsEnabled());
    }
}

String Node::getMidiProgramName (int program) const
{
    if (NodeObjectPtr obj = getObject())
        return obj->getMidiProgramName (program);
    return {};
}

void Node::setMidiProgramName (int program, const String& name)
{
    if (NodeObjectPtr obj = getObject())
        obj->setMidiProgramName (program, name);
    // setProperty (Tags::midiProgram, obj->areMidiProgramsEnabled());
}

NodeObjectSync::NodeObjectSync (const Node& node)
    : NodeObjectSync()
{
    setNode (node);
}

NodeObjectSync::NodeObjectSync()
{
    data.addListener (this);
}

NodeObjectSync::~NodeObjectSync()
{
    data.removeListener (this);
}

void NodeObjectSync::setNode (const Node& n)
{
    node = n;
    data.removeListener (this);
    data = node.getValueTree();
    data.addListener (this);
}

void NodeObjectSync::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    NodeObjectPtr obj = node.getObject();
    if (tree != data || frozen || obj == nullptr)
        return;

    if (property == Tags::midiChannels)
    {
        auto chans = node.getMidiChannels();
        obj->setMidiChannels (chans.get());
    }
    else if (property == Tags::keyStart)
    {
        ScopedFlag sf (frozen, true);
        auto start = roundToInt ((double) tree.getProperty (property));
        auto end = roundToInt ((double) tree.getProperty (Tags::keyEnd));
        if (end < start)
        {
            end = start;
            tree.setProperty (Tags::keyEnd, end, nullptr);
        }

        obj->setKeyRange (Range<int> (start, end));
    }
    else if (property == Tags::keyEnd)
    {
        ScopedFlag sf (frozen, true);
        auto end = roundToInt ((double) tree.getProperty (property));
        auto start = roundToInt ((double) tree.getProperty (Tags::keyStart));
        if (start > end)
        {
            start = end;
            tree.setProperty (Tags::keyStart, start, nullptr);
        }

        obj->setKeyRange (Range<int> (start, end));
    }
    else if (property == Tags::transpose)
    {
        obj->setTransposeOffset (roundToInt ((double) tree.getProperty (property)));
    }
    else if (property == Tags::delayCompensation)
    {
        obj->setDelayCompensation (tree.getProperty (property, obj->getDelayCompensation()));
        if (auto* const g = obj->getParentGraph())
        {
            g->cancelPendingUpdate();
            g->triggerAsyncUpdate();
        }
    }
}

void NodeObjectSync::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
{
    ignoreUnused (parent, child);
}

void NodeObjectSync::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index)
{
    ignoreUnused (parent, child, index);
}

void NodeObjectSync::valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex)
{
    ignoreUnused (parent, oldIndex, newIndex);
}

void NodeObjectSync::valueTreeParentChanged (ValueTree& tree)
{
    ignoreUnused (tree);
}

void NodeObjectSync::valueTreeRedirected (ValueTree& tree)
{
    ignoreUnused (tree);
}

} // namespace Element
