// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#include "nodes/baseprocessor.hpp" // for internal id macros
#include <element/node.hpp>
#include <element/session.hpp>
#include <element/script.hpp>

#include "engine/graphmanager.hpp"
#include "scopedflag.hpp"

namespace element {

//==============================================================================
struct NameSorter
{
    NameSorter() {}
    int compareElements (const Node& lhs, const Node& rhs)
    {
        return lhs.getName() < rhs.getName() ? -1 : lhs.getName() == rhs.getName() ? 0
                                                                                   : 1;
    }
    friend class NodeArray;
};

static void readPluginDescriptionForLoading (const ValueTree& p, PluginDescription& pd)
{
    const auto& type = p.getProperty (tags::type);

    if (type == types::Graph.toString())
    {
        pd.name = p.getProperty (tags::name);
        pd.fileOrIdentifier = EL_NODE_ID_GRAPH;
        pd.pluginFormatName = EL_NODE_FORMAT_NAME;
    }
    else
    {
        // plugins and io nodes
        pd.name = p.getProperty (tags::pluginName);
        pd.pluginFormatName = p.getProperty (tags::format);
        pd.fileOrIdentifier = p.getProperty (tags::identifier);
        if (pd.fileOrIdentifier.isEmpty())
            pd.fileOrIdentifier = p.getProperty (tags::file);
    }
}

static ValueTree getBlockValueTree (const Node& node)
{
    return node.getUIValueTree().getOrCreateChildWithName (types::Block, nullptr);
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
uint32 Port::index() const noexcept
{
    const int index = getProperty (tags::index, -1);
    return index >= 0 ? static_cast<uint32> (index) : EL_INVALID_PORT;
}

const String Port::symbol() const noexcept { return getProperty (tags::symbol, String()); }

int Port::channel() const noexcept
{
    const Node node (objectData.getParent().getParent());
    if (auto* g = node.getObject())
        return g->getChannelPort (index());
    return -1;
}

Node Port::getNode() const
{
    return Node (getNodeValueTree(), false);
}

void Port::setHiddenOnBlock (bool hidden)
{
    const auto node (getNode());
    auto symbols = getHiddenPortsProperty (node);

    if (hidden)
    {
        symbols.addIfNotAlreadyThere (symbol());
    }
    else
    {
        symbols.removeString (symbol().toRawUTF8());
    }

    setHiddenPortsProperty (node, symbols);
    setProperty ("hiddenOnBlock", hidden);
}

bool Port::isHiddenOnBlock() const
{
    const auto parent (getNode());
    if (! parent.isValid())
    {
        return true;
    }

    // TODO: hiddenOnBlock property can't be a 'once' object
    if (! objectData.hasProperty ("hiddenOnBlock"))
    {
        const auto symbols = getHiddenPortsProperty (parent);
        ValueTree (objectData).setProperty ("hiddenOnBlock", symbols.contains (symbol().toRawUTF8()), nullptr);
    }

    return (bool) getProperty ("hiddenOnBlock");
}

//==============================================================================
Node::Node() : Model() {}

Node::Node (const ValueTree& data, const bool setMissing)
    : Model (data)
{
    if (setMissing)
    {
        jassert (data.hasType (types::Node));
        setMissingProperties();
    }
}

// clang-format off
Node::Node (const Identifier& tp)
    : Model (types::Node, (tp == types::Graph ? EL_GRAPH_VERSION : EL_NODE_VERSION))
{
#if JUCE_DEBUG
    Array<Identifier> supported ({ types::Node, types::Graph });
    jassert (supported.contains (tp));
#endif
    objectData.setProperty (tags::type, tp.toString(), nullptr);
    setMissingProperties();
}
// clang-format on

Node::~Node() noexcept {}

//=============================================================================
bool Node::isValid() const noexcept { return objectData.hasType (types::Node); }
const String Node::getName() const noexcept { return getProperty (tags::name).toString(); }
void Node::setName (const String& name) { setProperty (tags::name, name); }

const String Node::getPluginName() const noexcept
{
    if (ProcessorPtr object = getObject())
        return object->getName();
    return {};
}

const String Node::getDisplayName() const noexcept
{
    String name = getName();
    if (name.isEmpty())
        name = getPluginName();
    return name;
}

bool Node::hasModifiedName() const noexcept
{
    auto dname = getName();
    return dname.isNotEmpty() && dname != getPluginName();
}

//=============================================================================
Node Node::createDefaultGraph (const String& name)
{
    Node graph (types::Graph);
    graph.setProperty (tags::name, name);
    ValueTree gports = graph.getPortsValueTree();

    int portIdx = 0;
    gports.addChild (Port ("Audio In 1", tags::audio, tags::input, portIdx++).data(), -1, 0);
    gports.addChild (Port ("Audio In 2", tags::audio, tags::input, portIdx++).data(), -1, 0);
    gports.addChild (Port ("MIDI In", tags::midi, tags::input, portIdx++).data(), -1, 0);
    gports.addChild (Port ("Audio Out 1", tags::audio, tags::output, portIdx++).data(), -1, 0);
    gports.addChild (Port ("Audio Out 2", tags::audio, tags::output, portIdx++).data(), -1, 0);
    gports.addChild (Port ("MIDI Out", tags::midi, tags::output, portIdx++).data(), -1, 0);

    ValueTree nodes = graph.getNodesValueTree();
    const auto types = StringArray ({ "audio.input", "audio.output", "midi.input", "midi.output" });
    const auto names = StringArray ({ "Audio In", "Audio Out", "MIDI In", "MIDI Out" });
    uint32 nodeId = 1;

    for (const auto& t : types)
    {
        ValueTree ioNode (types::Node);
        ValueTree ports = ioNode.getOrCreateChildWithName (tags::ports, 0);
        portIdx = 0;

        ioNode.setProperty (tags::id, static_cast<int64> (nodeId++), 0)
            .setProperty (tags::type, "plugin", 0)
            .setProperty (tags::format, "Internal", 0)
            .setProperty (tags::identifier, t, 0)
            .setProperty (tags::name, names[types.indexOf (t)], 0);

        if (t == "audio.input")
        {
            ioNode.setProperty (tags::relativeX, 0.25f, 0)
                .setProperty (tags::relativeY, 0.25f, 0)
                .setProperty ("numAudioIns", 0, 0)
                .setProperty ("numAudioOuts", 2, 0);

            Port port ("Port", tags::audio, tags::output, (uint32) portIdx++);
            ports.addChild (port.data(), -1, 0);
            port = Port ("Port", tags::audio, tags::output, (int) portIdx++);
            ports.addChild (port.data(), -1, 0);
        }
        else if (t == "audio.output")
        {
            ioNode.setProperty (tags::relativeX, 0.25f, 0)
                .setProperty (tags::relativeY, 0.75f, 0)
                .setProperty ("numAudioIns", 2, 0) // TODO: Needed?
                .setProperty ("numAudioOuts", 0, 0); // TODO: Needed?

            Port port ("Port", tags::audio, tags::input, (uint32) portIdx++);
            ports.addChild (port.data(), -1, 0);
            port = Port ("Port", tags::audio, tags::input, (int) portIdx++);
        }
        else if (t == "midi.input")
        {
            ioNode.setProperty (tags::relativeX, 0.75f, 0)
                .setProperty (tags::relativeY, 0.25f, 0)
                .setProperty ("numAudioIns", 0, 0)
                .setProperty ("numAudioOuts", 0, 0);
            Port port ("Port", tags::midi, tags::output, (uint32) portIdx++);
            ports.addChild (port.data(), -1, 0);
        }
        else if (t == "midi.output")
        {
            ioNode.setProperty (tags::relativeX, 0.75f, 0)
                .setProperty (tags::relativeY, 0.75f, 0)
                .setProperty ("numAudioIns", 0, 0)
                .setProperty ("numAudioOuts", 0, 0);
            Port port ("Port", tags::midi, tags::input, (uint32) portIdx++);
            ports.addChild (port.data(), -1, 0);
        }

        Node finalNode (ioNode, true);
        nodes.addChild (finalNode.data(), -1, 0);
    }

    return graph;
}

bool Node::isProbablyGraphNode (const ValueTree& data)
{
    // clang-format off
    const var& tp = data.getProperty (tags::type);
    return (data.hasType (types::Node) || data.hasType (tags::node)) &&
        (tags::graph.toString() == tp.toString() ||
         types::Graph.toString() == tp.toString() ||
         String ("default") == tp.toString());
    // clang-format on
}

ValueTree Node::resetIds (const ValueTree& data)
{
    ValueTree result = data;
    jassert (result.hasType (types::Node)); // must be a node
    jassert (! result.getParent().isValid()); // cannot be part of another object tree
    if (result.getParent().isValid())
        return result;
    result.removeProperty (tags::id, nullptr);
    result.setProperty (tags::uuid, Uuid().toString(), nullptr);
    return result;
}

ValueTree Node::parse (const File& file)
{
    ValueTree sessionData = Session::readFromFile (file);
    if (sessionData.isValid())
    {
        const auto graphs = sessionData.getChildWithName (tags::graphs);
        const auto sessionNode = graphs.getChild (graphs.getProperty (tags::active, 0));
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

    if (data.hasType (types::Node))
    {
        nodeData = data;
    }
    else
    {
        nodeData = data.getChildWithName (types::Node);
        if (! nodeData.isValid())
        {
            nodeData = data.getChildWithName (tags::node);
            if (nodeData.isValid())
            {
                String error;
                nodeData = Node::migrate (nodeData, error);
                if (error.isNotEmpty())
                {
                    std::clog << "[element] error: " << error.toStdString() << std::endl;
                }
            }
        }

        // copy properties from preset if needed.
        if (nodeData.isValid())
        {
            // Rename the node appropriately
            if (data.hasProperty (tags::name))
                nodeData.setProperty (tags::name, data.getProperty (tags::name), 0);
            else
                nodeData.setProperty (tags::name, file.getFileNameWithoutExtension(), 0);
        }
    }

    if (nodeData.isValid() && nodeData.hasType (types::Node))
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
    node.removeProperty (tags::updater, nullptr);
    node.removeProperty (tags::object, nullptr);

    if (node.hasType (types::Node))
    {
        Array<Identifier> properties ({ tags::offline,
                                        tags::placeholder,
                                        tags::missing });
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
        return e->writeTo (targetFile);
#endif

    return false;
}

bool Node::savePresetTo (const DataPath& path, const String& name) const
{
    {
        // hack: ensure the plugin's state info is up-to-date
        Node (*this).savePluginState();
    }

    ValueTree preset (tags::preset);
    ValueTree data = objectData.createCopy();
    sanitizeProperties (data, true);
    preset.addChild (data, -1, 0);

    const auto targetFile = path.getPresetFile (name);
    if (! targetFile.getParentDirectory().exists())
        targetFile.getParentDirectory().createDirectory();

    data.setProperty (tags::version, EL_NODE_VERSION, 0);
    data.setProperty (tags::name, targetFile.getFileNameWithoutExtension(), 0);
    data.setProperty (tags::type, tags::node.toString(), 0);

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
        return e->writeTo (targetFile);
#endif
    return false;
}

Node Node::createGraph (const String& name)
{
    Node node (types::Graph);
    ValueTree data = node.data();
    data.setProperty (tags::name, name, nullptr);
    data.getOrCreateChildWithName (tags::nodes, nullptr);
    data.getOrCreateChildWithName (tags::arcs, nullptr);
    return node;
}

ValueTree Node::makeArc (const Arc& arc)
{
    ValueTree model (types::Arc);
    model.setProperty (tags::sourceNode, (int) arc.sourceNode, nullptr);
    model.setProperty (tags::sourcePort, (int) arc.sourcePort, nullptr);
    model.setProperty (tags::destNode, (int) arc.destNode, nullptr);
    model.setProperty (tags::destPort, (int) arc.destPort, nullptr);
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
    ValueTree tmp = objectData.getParent();
    if (tmp.hasType (tags::nodes))
        tmp = tmp.getParent();
    if (! tmp.isValid())
        return ValueTree();

    jassert (tmp.hasType (types::Node));
    return tmp.getOrCreateChildWithName (tags::arcs, nullptr);
}

void Node::getPluginDescription (PluginDescription& p) const
{
    readPluginDescriptionForLoading (objectData, p);
}

ValueTree Node::addScript (const Script& script)
{
    auto data = script.data();
    if (data.getParent().isValid())
        data = data.createCopy();
    Script src (data);
    if (src.valid())
        getScriptsValueTree().addChild (src.data(), -1, nullptr);
    return src.data();
}

void Node::setMissingProperties()
{
    stabilizePropertyString (tags::uuid, Uuid().toString());
    stabilizePropertyString (tags::type, types::Node.toString());
    stabilizePropertyString (tags::name, "Node");
    stabilizeProperty (tags::bypass, false);
    stabilizeProperty (tags::persistent, true);
    stabilizePropertyString (tags::renderMode, "single");
    stabilizeProperty (tags::keyStart, 0);
    stabilizeProperty (tags::keyEnd, 127);
    stabilizeProperty (tags::transpose, 0);
    stabilizeProperty (tags::delayCompensation, 0);
    stabilizeProperty (tags::tempo, (double) 120.0);
    objectData.getOrCreateChildWithName (tags::nodes, nullptr);
    objectData.getOrCreateChildWithName (tags::ports, nullptr);
    objectData.getOrCreateChildWithName (tags::scripts, nullptr);
    auto ui = objectData.getOrCreateChildWithName (tags::ui, nullptr);
    ui.getOrCreateChildWithName (types::Block, nullptr);
}

Processor* Node::getObject() const
{
    return dynamic_cast<Processor*> (objectData.getProperty (tags::object, var()).getObject());
}

Processor* Node::getObjectForId (const uint32 nodeId) const
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
// clang-format off
bool Node::isAudioIONode() const
{
    return objectData.getProperty (tags::format) == "Internal" &&
        (objectData.getProperty (tags::identifier) == "audio.input" ||
         objectData.getProperty (tags::identifier) == "audio.output");
}

bool Node::isAudioInputNode() const
{
    return objectData.getProperty (tags::format) == "Internal" &&
        objectData.getProperty (tags::identifier) == "audio.input";
}

bool Node::isAudioOutputNode() const
{
    return objectData.getProperty (tags::format) == "Internal" &&
        objectData.getProperty (tags::identifier) == "audio.output";
}

bool Node::isMidiIONode() const
{
    return objectData.getProperty (tags::format) == "Internal" &&
        (objectData.getProperty (tags::identifier) == "midi.input" ||
         objectData.getProperty (tags::identifier) == "midi.output");
}

/** Returns true if a global MIDI input node. e.g */
bool Node::isMidiInputNode() const
{
    return objectData.getProperty (tags::format) == "Internal" &&
        objectData.getProperty (tags::identifier) == "midi.input";
}

/** Returns true if a global MIDI output node. e.g */
bool Node::isMidiOutputNode() const
{
    return objectData.getProperty (tags::format) == "Internal" &&
        objectData.getProperty (tags::identifier) == "midi.output";
}
// clang-format on

//==============================================================================
bool Node::isMidiInputDevice() const
{
    return objectData.getProperty (tags::format) == EL_NODE_FORMAT_NAME && objectData.getProperty (tags::identifier) == EL_NODE_ID_MIDI_INPUT_DEVICE;
}

bool Node::isMidiOutputDevice() const
{
    return objectData.getProperty (tags::format) == EL_NODE_FORMAT_NAME && objectData.getProperty (tags::identifier) == EL_NODE_ID_MIDI_OUTPUT_DEVICE;
}

//==============================================================================
void Node::resetPorts()
{
    if (ProcessorPtr ptr = getObject())
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
    if (! nodes.hasType (tags::nodes))
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
    if (! nodes.hasType (tags::nodes))
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
    Arc arc ((uint32) (int) data.getProperty (tags::sourceNode, (int) EL_INVALID_NODE),
             (uint32) (int) data.getProperty (tags::sourcePort, (int) EL_INVALID_PORT),
             (uint32) (int) data.getProperty (tags::destNode, (int) EL_INVALID_NODE),
             (uint32) (int) data.getProperty (tags::destPort, (int) EL_INVALID_PORT));
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
        if (static_cast<int> (sourceNode) == (int) arc.getProperty (tags::sourceNode) && static_cast<int> (sourcePort) == (int) arc.getProperty (tags::sourcePort) && static_cast<int> (destNode) == (int) arc.getProperty (tags::destNode) && static_cast<int> (destPort) == (int) arc.getProperty (tags::destPort))
        {
            return (checkMissing) ? ! arc.getProperty (tags::missing, false) : true;
        }
    }

    return false;
}

Node Node::getNodeById (const uint32 nodeId) const
{
    const ValueTree nodes = getNodesValueTree();
    Node node (nodes.getChildWithProperty (tags::id, static_cast<int64> (nodeId)), false);
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
        Node node (nodes.getChildWithProperty (tags::uuid, uuid.toString()), false);
        return node;
    }

    return findNodeRecursive (*this, uuid);
}

Port Node::getPort (const int index) const
{
    Port port (getPortsValueTree().getChild (index));
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
    setProperty (tags::relativeX, x);
    setProperty (tags::relativeY, y);
}

void Node::getRelativePosition (double& x, double& y) const
{
    x = (double) getProperty (tags::relativeX, 0.5f);
    y = (double) getProperty (tags::relativeY, 0.5f);
}

bool Node::hasPosition() const
{
    return hasProperty (tags::x) && hasProperty (tags::y);
}

void Node::getPosition (double& x, double& y) const
{
    x = (double) getProperty (tags::x, 0.0);
    y = (double) getProperty (tags::y, 0.0);
}

void Node::setPosition (double x, double y)
{
    setProperty (tags::x, x);
    setProperty (tags::y, y);
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

MidiChannels Node::getMidiChannels() const
{
    MidiChannels chans;
    if (objectData.hasProperty (tags::midiChannels))
    {
        if (auto* const block = objectData.getProperty (tags::midiChannels).getBinaryData())
        {
            BigInteger data;
            data.loadFromMemoryBlock (*block);
            chans.setChannels (data);
        }
    }
    else
    {
        const auto channel = (int) objectData.getProperty (tags::midiChannel, 0);
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

    if (ProcessorPtr obj = getObject())
    {
        if (auto* const proc = obj->getAudioProcessor())
        {
            const int wantedProgram = objectData.getProperty (tags::program, -1);
            const bool shouldSetProgram = proc->getNumPrograms() > 0 && isPositiveAndBelow (wantedProgram, proc->getNumPrograms());
            if (shouldSetProgram)
                proc->setCurrentProgram (wantedProgram);

            auto data = getProperty (tags::state).toString().trim();
            if (data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                {
                    proc->setStateInformation (state.getData(), (int) state.getSize());
                }
            }

            data = getProperty (tags::programState).toString().trim();
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
            const int wantedProgram = objectData.getProperty (tags::program, -1);
            const bool shouldSetProgram = obj->getNumPrograms() > 0 && isPositiveAndBelow (wantedProgram, obj->getNumPrograms());
            if (shouldSetProgram)
                obj->setCurrentProgram (wantedProgram);

            auto data = getProperty (tags::state).toString().trim();
            if (data.isNotEmpty())
            {
                MemoryBlock state;
                state.fromBase64Encoding (data);
                if (state.getSize() > 0)
                    obj->setState (state.getData(), (int) state.getSize());
            }
        }

        if (hasProperty (tags::bypass))
        {
            obj->suspendProcessing (isBypassed());
        }

        if (hasProperty (tags::gain))
        {
            obj->setGain (getProperty ("gain"));
        }

        if (hasProperty ("inputGain"))
        {
            obj->setInputGain (getProperty ("inputGain"));
        }

        if (hasProperty (tags::keyStart) && hasProperty (tags::keyEnd))
        {
            Range<int> range (getProperty (tags::keyStart, 0),
                              getProperty (tags::keyEnd, 127));
            obj->setKeyRange (range);
        }

        if (hasProperty (tags::midiChannels))
        {
            const MidiChannels channels (getMidiChannels());
            obj->setMidiChannels (channels.get());
        }

        if (hasProperty (tags::midiProgram))
        {
            obj->setMidiProgram ((int) getProperty (tags::midiProgram, -1));
        }

        if (hasProperty (tags::midiProgramsEnabled))
            obj->setMidiProgramsEnabled ((bool) getProperty (tags::midiProgramsEnabled, true));
        obj->setUseGlobalMidiPrograms ((bool) getProperty (tags::globalMidiPrograms, obj->useGlobalMidiPrograms()));
        if (hasProperty (tags::midiProgramsState))
            obj->setMidiProgramsState (getProperty (tags::midiProgramsState).toString().trim());

        obj->setMuted ((bool) getProperty (tags::mute, obj->isMuted()));
        obj->setMuteInput ((bool) getProperty ("muteInput", obj->isMutingInputs()));

        if (hasProperty (tags::transpose))
            obj->setTransposeOffset (getProperty (tags::transpose));

        obj->setOversamplingFactor (jmax (1, (int) getProperty (tags::oversamplingFactor, 1)));
        obj->setDelayCompensation (getProperty (tags::delayCompensation, 0.0));
    }

    // this was originally here to help reduce memory usage
    // need another way to free this property without disturbing
    // the normal flow of the app.
    const bool clearStateProperty = false;
    if (clearStateProperty)
        objectData.removeProperty (tags::state, 0);

    for (int i = 0; i < getNumNodes(); ++i)
        getNode (i).restorePluginState();
}

void Node::savePluginState()
{
    if (! isValid())
        return;

    ProcessorPtr obj = getObject();
    if (obj && obj->isPrepared)
    {
        MemoryBlock state;

        if (auto* proc = obj->getAudioProcessor())
        {
            proc->getStateInformation (state);
            if (state.getSize() > 0)
            {
                objectData.setProperty (tags::state, state.toBase64Encoding(), nullptr);
            }
            else
            {
                const bool clearStateProperty = false;
                if (clearStateProperty)
                    objectData.removeProperty (tags::state, 0);
            }

            state.reset();
            proc->getCurrentProgramStateInformation (state);
            if (state.getSize() > 0)
            {
                objectData.setProperty (tags::programState, state.toBase64Encoding(), 0);
            }

            setProperty (tags::bypass, proc->isSuspended());
            setProperty (tags::program, proc->getCurrentProgram());

            const auto layout = proc->getBusesLayout();
            auto buses = objectData.getOrCreateChildWithName (tags::buses, nullptr);
            buses.removeAllChildren (nullptr);

            auto channelSetToData = [] (const AudioChannelSet& acs) -> ValueTree {
                ValueTree bus (types::AudioChannelSet);
                bus.setProperty (tags::arrangement, acs.getSpeakerArrangementAsString(), nullptr);
                return bus;
            };

            auto bins = buses.getOrCreateChildWithName (tags::inputs, nullptr);
            for (int i = 0; i < layout.inputBuses.size(); ++i)
            {
                auto data = channelSetToData (layout.inputBuses.getReference (i));
                if (data.isValid())
                    bins.addChild (data, -1, nullptr);
            }

            auto bouts = buses.getOrCreateChildWithName (tags::outputs, nullptr);
            for (int i = 0; i < layout.outputBuses.size(); ++i)
            {
                auto data = channelSetToData (layout.outputBuses.getReference (i));
                if (data.isValid())
                    bouts.addChild (data, -1, nullptr);
            }
        }
        else
        {
            obj->getState (state);
            if (state.getSize() > 0)
                objectData.setProperty (tags::state, state.toBase64Encoding(), nullptr);
        }

        setProperty (tags::midiProgram, obj->getMidiProgram());
        setProperty (tags::globalMidiPrograms, obj->useGlobalMidiPrograms());
        setProperty (tags::midiProgramsEnabled, obj->areMidiProgramsEnabled());
        setProperty (tags::mute, obj->isMuted());
        setProperty ("muteInput", obj->isMutingInputs());
        String mps;
        obj->getMidiProgramsState (mps);
        setProperty (tags::midiProgramsState, mps);
        setProperty (tags::oversamplingFactor, obj->getOversamplingFactor());
        setProperty (tags::delayCompensation, obj->getDelayCompensation());
    }

    for (int i = 0; i < getNumNodes(); ++i)
        getNode (i).savePluginState();
}

namespace detail {

static ValueTree migrateNode (const ValueTree& data, String& error)
{
    const Model model (data);
    if (! model.hasType (types::Node) && ! model.hasType (tags::node))
    {
        error = "data is not a node: el.";
        error << model.getType().toString();
        return {};
    }

    if (model.version() > EL_NODE_VERSION)
    {
        error = "Node rollback from newer to older version not supported";
        return {};
    }

    if (model.version() == EL_NODE_VERSION)
        return data.createCopy();

    ValueTree newData;
    if (model.version() == 0)
    {
        Identifier newType = types::Node;
        if (model.getProperty (tags::type).toString() == tags::graph.toString())
            newType = types::Graph;

        newData = ValueTree (types::Node);
        newData.copyPropertiesFrom (data, nullptr);
        newData.setProperty (tags::type, newType.toString(), nullptr);
        Node (newData, true);

        {
            auto o = data.getChildWithName (tags::arcs);
            auto n = newData.getOrCreateChildWithName (tags::arcs, 0);
            Model::copyChildrenWithType (o, n, types::Arc);
        }
        {
            auto o = data.getChildWithName (tags::ports);
            auto n = newData.getOrCreateChildWithName (tags::ports, 0);
            Model::copyChildrenWithType (o, n, types::Port);
        }
        {
            auto o = data.getChildWithName (tags::ui);
            auto n = newData.getOrCreateChildWithName (tags::ui, 0);
            n.copyPropertiesAndChildrenFrom (o, nullptr);

            const auto ob = o.getChildWithName (tags::block);
            if (ob.isValid())
            {
                auto newBlock = n.getChildWithName (tags::block);
                if (newBlock.isValid())
                    n.removeChild (newBlock, 0);
                newBlock = n.getChildWithName (types::Block);
                if (newBlock.isValid())
                    n.removeChild (newBlock, 0);

                newBlock = Model::copyWithType (ob, types::Block);
                if (newBlock.isValid())
                {
                    n.addChild (newBlock, -1, 0);
                }
            }
        }
        {
            auto o = data.getChildWithName (tags::scripts);
            auto n = newData.getOrCreateChildWithName (tags::scripts, 0);
            Model::copyChildrenWithType (o, n, types::Script);
        }

        Node newNode (newData, false);

        // version 0 nodes did not show control ports therefore
        // need to be set to the default block visibility = hidden
        for (int i = 0; i < newNode.getNumPorts(); ++i)
        {
            auto port = newNode.getPort (i);
            if (port.getType() == PortType::Audio || port.getType() == PortType::Midi)
                continue;
            port.setHiddenOnBlock (true);
        }

        const bool isGraph = newNode.isGraph();
        auto oldNodes = data.getChildWithName (tags::nodes);
        auto newNodes = newData.getOrCreateChildWithName (tags::nodes, nullptr);
        auto newPorts = newData.getOrCreateChildWithName (tags::ports, nullptr);
        PortArray newGraphIO;
        for (const auto& child : oldNodes)
        {
            auto cm = detail::migrateNode (child, error);
            if (cm.hasType (types::Node) && error.isEmpty())
            {
                const Node cmn (cm, false);
                newNodes.addChild (cm.createCopy(), -1, nullptr);
                if (isGraph && cmn.isDuplex())
                {
                    if (cmn.isAudioInputNode())
                    {
                        cmn.getPorts (newGraphIO, PortType::Audio, false);
                    }
                    else if (cmn.isAudioOutputNode())
                    {
                        cmn.getPorts (newGraphIO, PortType::Audio, true);
                    }
                    else if (cmn.isMidiInputNode())
                    {
                        cmn.getPorts (newGraphIO, PortType::Midi, false);
                    }
                    else if (cmn.isMidiOutputNode())
                    {
                        cmn.getPorts (newGraphIO, PortType::Midi, true);
                    }
                }
            }
            else
            {
                if (error.isEmpty())
                    error = "Unexpected node child error.";
                return {};
            }
        }

        if (! newGraphIO.isEmpty())
        {
            for (auto& gport : newGraphIO)
            {
                // flip direction for sanity reason at the graph level.
                gport.setProperty (tags::flow, gport.isInput() ? tags::output.toString() : tags::input.toString());
                newPorts.addChild (gport.data(), -1, nullptr);
            }
            newGraphIO.clear();
        }
    }
    else
    {
        newData = {};
        error = "Node migrations not implemented for data version: ";
        error << model.version();
    }

    return newData;
}

} // namespace detail

ValueTree Node::migrate (const ValueTree& data, String& error) noexcept
{
    error.clear();
    Model model (data);
    if (model.hasType (types::Node) && model.version() == EL_NODE_VERSION)
        return data.createCopy();
    auto newData = detail::migrateNode (data, error);
    newData = error.isEmpty() ? newData : ValueTree();
    if (! newData.isValid() && error.isEmpty())
        error = "Unexpected node migration problem.";

    if (error.isNotEmpty())
    {
        std::clog << newData.toXmlString().toStdString() << std::endl;
    }

    return newData;
}

void Node::setMuted (bool shouldBeMuted)
{
    if (shouldBeMuted != isMuted())
        setProperty (tags::mute, shouldBeMuted);
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

void Node::customizePresetsPopupMenu (juce::PopupMenu& menuToAddTo)
{
    if (auto* obj = getObject())
        obj->customizePresetsPopupMenu (menuToAddTo);
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
    if (tags::plugin == getNodeType())
        if (auto gn = getObject())
            if (auto* const proc = gn->getAudioProcessor())
                return proc->hasEditor();
    return false;
}

void ConnectionBuilder::addConnections (GraphManager& controller, const uint32 targetNodeId) const
{
    ProcessorPtr tgt = controller.getNodeForId (targetNodeId);
    if (tgt)
    {
        bool anythingAdded = false;
        for (const auto* pc : portChannelMap)
        {
            ProcessorPtr ptr = controller.getNodeForId (pc->nodeId);
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

// default value here must match that as defined in Processor.h
bool Node::useGlobalMidiPrograms() const { return (bool) getProperty (tags::globalMidiPrograms, false); }
void Node::setUseGlobalMidiPrograms (bool useGlobal)
{
    if (ProcessorPtr obj = getObject())
    {
        if (obj->useGlobalMidiPrograms() == useGlobal)
            return;
        obj->setUseGlobalMidiPrograms (useGlobal);
        setProperty (tags::globalMidiPrograms, obj->useGlobalMidiPrograms());
    }
}

// default value here must match that as defined in Processor.h
bool Node::areMidiProgramsEnabled() const { return (bool) getProperty (tags::midiProgramsEnabled, false); }
void Node::setMidiProgramsEnabled (bool useMidiPrograms)
{
    if (ProcessorPtr obj = getObject())
    {
        if (obj->areMidiProgramsEnabled() == useMidiPrograms)
            return;
        obj->setMidiProgramsEnabled (useMidiPrograms);
        setProperty (tags::midiProgramsEnabled, obj->areMidiProgramsEnabled());
    }
}

int Node::getMidiProgram() const { return (int) getProperty (tags::midiProgram, 0); }
void Node::setMidiProgram (int program)
{
    if (ProcessorPtr obj = getObject())
    {
        if (obj->getMidiProgram() == program)
            return;
        obj->setMidiProgram (program);
        setProperty (tags::midiProgram, obj->areMidiProgramsEnabled());
    }
}

String Node::getMidiProgramName (int program) const
{
    if (ProcessorPtr obj = getObject())
        return obj->getMidiProgramName (program);
    return {};
}

void Node::setMidiProgramName (int program, const String& name)
{
    if (ProcessorPtr obj = getObject())
        obj->setMidiProgramName (program, name);
    // setProperty (tags::midiProgram, obj->areMidiProgramsEnabled());
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
    data = node.data();
    data.addListener (this);
}

void NodeObjectSync::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
{
    ProcessorPtr obj = node.getObject();
    if (tree != data || frozen || obj == nullptr)
        return;

    if (property == tags::midiChannels)
    {
        auto chans = node.getMidiChannels();
        obj->setMidiChannels (chans.get());
    }
    else if (property == tags::keyStart)
    {
        ScopedFlag sf (frozen, true);
        auto start = roundToInt ((double) tree.getProperty (property));
        auto end = roundToInt ((double) tree.getProperty (tags::keyEnd));
        if (end < start)
        {
            end = start;
            tree.setProperty (tags::keyEnd, end, nullptr);
        }

        obj->setKeyRange (Range<int> (start, end));
    }
    else if (property == tags::keyEnd)
    {
        ScopedFlag sf (frozen, true);
        auto end = roundToInt ((double) tree.getProperty (property));
        auto start = roundToInt ((double) tree.getProperty (tags::keyStart));
        if (start > end)
        {
            start = end;
            tree.setProperty (tags::keyStart, start, nullptr);
        }

        obj->setKeyRange (Range<int> (start, end));
    }
    else if (property == tags::transpose)
    {
        obj->setTransposeOffset (roundToInt ((double) tree.getProperty (property)));
    }
    else if (property == tags::delayCompensation)
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

} // namespace element
