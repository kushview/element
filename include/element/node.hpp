// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL3-or-later

#pragma once

#include <element/arc.hpp>
#include <element/model.hpp>
#include <element/midichannels.hpp>
#include <element/nodeobject.hpp>
#include <element/tags.hpp>

namespace element {

// FIXME:
using namespace juce;

class DataPath;
class GraphManager;
class NodeArray;
class PortArray;
class Node;
class Script;

/** Representation of a Port of a Node. */
class Port : public Model {
public:
    Port() : Model (tags::port) {}
    Port (const ValueTree& p)
        : Model (p) { jassert (p.hasType (tags::port)); }
    ~Port() {}

    /** Returns the ValueTree of the Node containing this port
        will not always be valid 
     */
    inline ValueTree getNodeValueTree() const { return objectData.getParent().getParent(); }

    /** Returns the Node containing this port */
    Node getNode() const;

    /** Returns true if this port probably lives on a Node */
    inline bool hasParentNode() const { return getNodeValueTree().hasType (tags::node); }

    const bool isInput() const
    {
        jassert (objectData.hasProperty ("flow"));
        return getProperty ("flow", "").toString() == "input";
    }

    const bool isOutput() const
    {
        jassert (objectData.hasProperty ("flow"));
        return getProperty ("flow", "").toString() == "output";
    }

    /** Returns the port name. */
    const String getName() const { return getProperty (tags::name, "Port"); }

    /** Returns the type of this Port. */
    const PortType getType() const { return PortType (getProperty (tags::type, "unknown").toString()); }

    bool isA (const PortType type, const bool isInputFlow) const { return getType() == type && isInputFlow == isInput(); }

    /** Returns the port index of this Port. */
    uint32 index() const noexcept;

    /** Returns the symbol for this Port. */
    const String symbol() const noexcept;

    /** Returns the coresponding channel for this port's index */
    int channel() const noexcept;

    /** Change block visibility. */
    void setHiddenOnBlock (bool visible);

    /** Returns true if should be hidden on block. */
    bool isHiddenOnBlock() const;
};

/** Representation of a Node */
class Node : public Model {
public:
    /** Create an invalid node */
    Node();

    /** Create a node with existing data */
    Node (const ValueTree& data, const bool setMissing = true);

    /** Creates a node with specific type */
    Node (const Identifier& nodeType);

    /** Destructor */
    ~Node() noexcept;

    /** Returns true if the connection exists in the provided ValueTree

        @param arcs          Value tree containing connections
        @param sourceNode    The source node ID
        @param sourcePort    The source port index
        @param destNode      The target node ID
        @param destPort      The target port index
        @param checkMissing  If true, will return false if found but has the missing property
    */
    static bool connectionExists (const ValueTree& arcs, const uint32 sourceNode, const uint32 sourcePort, const uint32 destNode, const uint32 destPort, const bool checkMissing = false);

    /** Creates a default graph structure with optional name */
    static Node createDefaultGraph (const String& name = String());

    /** Creates an empty graph model */
    static Node createGraph (const String& name = String());

    /** Returns true if the value tree is probably a graph node */
    static bool isProbablyGraphNode (const ValueTree& data);

    /** Removes unused id properties and resets the uuid */
    static ValueTree resetIds (const ValueTree& data);

    /** Load node data from file */
    static ValueTree parse (const File& file);

    /** Removes properties that can't be saved to a file. e.g. object properties */
    static void sanitizeProperties (ValueTree node, const bool recursive = false);

    /** This is just an alias right now */
    static void sanitizeRuntimeProperties (ValueTree node, const bool recursive = false);

    /** Create a value tree version of an arc */
    static ValueTree makeArc (const Arc& arc);

    /** Create an Arc from a ValueTree */
    static Arc arcFromValueTree (const ValueTree& data);

    //=========================================================================
    /** Returns true if the underlying data is probably a node */
    bool isValid() const { return objectData.hasType (tags::node); }

    /** Returns the user-modifiable name of this node */
    const String getName() const { return getProperty (tags::name); }

    /** Change the user-defined name */
    void setName (const String& name) { setProperty (tags::name, name); }

    /** Returns the node name defined by the user. If not set it returns the
        node name set when loaded.
    */
    const String getDisplayName() const;

    /** Returns the name as defined by the loaded plugin */
    const String getPluginName() const;

    /** Returns true if the name has been user-modified */
    bool hasModifiedName() const
    {
        auto dname = getName();
        return dname.isNotEmpty() && dname != getPluginName();
    }

    //=========================================================================
    /** Returns the nodeId as defined in the engine */
    const uint32 getNodeId() const { return (uint32) (int64) getProperty (tags::id); }

    /** Returns this Node's UUID as a string */
    String getUuidString() const { return objectData.getProperty (tags::uuid).toString(); }

    /** Returns this Node's UUID */
    Uuid getUuid() const { return Uuid (getUuidString()); }

    //=========================================================================
    /** Returns true if this node is probably a graph */
    bool isGraph() const { return isProbablyGraphNode (objectData); }

    /** Returns true if this is a root graph on the session */
    bool isRootGraph() const
    {
        return objectData.getParent().hasType (tags::graphs) && objectData.getParent().getParent().hasType (tags::session);
    }

    /** Returns an Identifier indicating this nodes type */
    const Identifier getNodeType() const
    {
        auto type = getProperty (tags::type).toString();
        return type.isNotEmpty() ? type : Identifier ("unknown");
    }

    /** Returns true if this node is a specific type */
    const bool hasNodeType (const Identifier& t) const { return getNodeType() == t; }

    /** Returns true if this node is of the given format and unique ID */
    bool isA (const String& format, const String& identifier) const;

    //=========================================================================
    /** Returns true if this node has a custom editor */
    bool hasEditor() const;

    //=========================================================================
    /** Returns the graph containing this node, if any */
    Node getParentGraph() const;

    /** Returns true if descendent of */
    bool descendsFrom (const Node& graph) const;

    /** Returns true if this is a direct child of a root level graph */
    bool isChildOfRootGraph() const;

    //=========================================================================
    /** Returns true if this node couldn't be loaded but still remains on it's
        parenet graph */
    bool isMissing() const { return hasProperty (tags::missing); }

    /** Returns true if this node should be enabled */
    inline const bool isEnabled() const { return (bool) getProperty (tags::enabled, true); }

    //=========================================================================
    /** Returns the enabled MIDI channels on this Node */
    MidiChannels getMidiChannels() const;

    //=========================================================================
    /** Returns true if bypass is on for this Node */
    bool isBypassed() const { return objectData.getProperty (tags::bypass, false); }

    /** Returns the Value object for the bypass property */
    Value getBypassedValue() { return getPropertyAsValue (tags::bypass); }

    //=========================================================================
    /** Returns true if this Node is muted */
    bool isMuted() const { return (bool) getProperty (tags::mute, false); }

    /** Returns the Value object for the mute property */
    Value getMutedValue() { return getPropertyAsValue (tags::mute); }

    /** Returns true if inputs are muted */
    bool isMutingInputs() const { return (bool) getProperty ("muteInput", false); }

    /** Change the mute status of this Node */
    void setMuted (bool);

    /** Change the mute status of inputs on this Node */
    void setMuteInput (bool);

    //=========================================================================
    /** Returns the number of connections on this node */
    int getNumConnections() const;

    /** Returns a Value tree containing connection information */
    ValueTree getConnectionValueTree (const int index) const;

    /** Get an array of Arcs contained on this Node (graph) */
    void getArcs (OwnedArray<Arc>&) const;

    //=========================================================================
    /** Set relative position */
    void setRelativePosition (const double x, const double y);

    /** Gets the x/y relative coordinates of this node */
    void getRelativePosition (double& x, double& y) const;

    /** Returns true if properties "x" and "y" are present */
    bool hasPosition() const;

    /** Gets the x/y coordinates of this node in the editor */
    void getPosition (double& x, double& y) const;

    /** Set the x/y position of this node in the editor */
    void setPosition (double x, double y);

    //=========================================================================
    /** Returns the engine-side graphnode implementation */
    NodeObject* getObject() const;

    /** Returns a child graph node object by id */
    NodeObject* getObjectForId (const uint32) const;

    /** Returns true if this node can connect to another in any capacity */
    const bool canConnectTo (const Node& o) const;

    //=========================================================================
    /** Returns the number of child nodes contained on this node */
    int getNumNodes() const { return getNodesValueTree().getNumChildren(); }

    /** Returns a child node by index */
    Node getNode (const int index) const { return Node (getNodesValueTree().getChild (index), false); }

    const int getNumAudioIns() const { return (int) getProperty ("numAudioIns", 0); }
    const int getNumAudioOuts() const { return (int) getProperty ("numAudioOuts", 0); }

    int getNumPorts() const { return getPortsValueTree().getNumChildren(); }
    void getPorts (PortArray& ports, PortType type, bool isInput) const;
    void getPorts (PortArray& ins, PortArray& outs, PortType type) const;
    void getAudioInputs (PortArray& ports) const;
    void getAudioOutputs (PortArray& ports) const;

    inline bool isAudioIONode() const
    {
        return objectData.getProperty (tags::format) == "Internal" && (objectData.getProperty (tags::identifier) == "audio.input" || objectData.getProperty (tags::identifier) == "audio.output");
    }

    inline bool isAudioInputNode() const
    {
        return objectData.getProperty (tags::format) == "Internal" && objectData.getProperty (tags::identifier) == "audio.input";
    }

    inline bool isAudioOutputNode() const
    {
        return objectData.getProperty (tags::format) == "Internal" && objectData.getProperty (tags::identifier) == "audio.output";
    }

    inline bool isMidiIONode() const
    {
        return objectData.getProperty (tags::format) == "Internal" && (objectData.getProperty (tags::identifier) == "midi.input" || objectData.getProperty (tags::identifier) == "midi.output");
    }

    /** Returns true if a global MIDI input node. e.g */
    inline bool isMidiInputNode() const
    {
        return objectData.getProperty (tags::format) == "Internal" && objectData.getProperty (tags::identifier) == "midi.input";
    }

    /** Returns true if a global MIDI output node. e.g */
    inline bool isMidiOutputNode() const
    {
        return objectData.getProperty (tags::format) == "Internal" && objectData.getProperty (tags::identifier) == "midi.output";
    }

    inline bool isIONode() const { return isAudioIONode() || isMidiIONode(); }

    /** Returns true if this is a single MIDI input device (not global) */
    bool isMidiInputDevice() const;

    /** Returns true if this is a single MIDI output device (not global) */
    bool isMidiOutputDevice() const;

    /** Returns true if a MIDI in or out device */
    bool isMidiDevice() { return isMidiInputDevice() || isMidiOutputDevice(); }

    /** Returns the format of this node */
    inline const var& getFormat() const { return objectData.getProperty (tags::format); }

    /** Returns this nodes identifier */
    inline const var& getIdentifier() const { return objectData.getProperty (tags::identifier); }

    /** Returns a file property if exists, otherwise the identifier property */
    inline const var& getFileOrIdentifier() const
    {
        return objectData.hasProperty (tags::file)
                   ? objectData.getProperty (tags::file)
                   : getIdentifier();
    }

    /** returns the first node by format and identifier */
    inline Node getNodeByFormat (const var& format, const var& identifier) const
    {
        auto nodes = getNodesValueTree();

        for (int i = 0; i < nodes.getNumChildren(); ++i) {
            auto child = nodes.getChild (i);
            if (child[tags::format] == format && child[tags::identifier] == identifier)
                return Node (child, false);
        }

        return Node();
    }

    Node getIONode (PortType portType, const bool isInput) const
    {
        if (! portType.isAudio() && ! portType.isMidi())
            return Node();

        String identifier = portType.getSlug();
        identifier << "." << String ((isInput) ? "input" : "output");
        return getNodeByFormat ("Internal", identifier);
    }

    bool hasChildNode (const var& format, const var& identifier) const
    {
        auto nodes = getNodesValueTree();
        for (int i = 0; i < nodes.getNumChildren(); ++i) {
            auto child = nodes.getChild (i);
            if (child[tags::format] == format && child[tags::identifier] == identifier)
                return true;
        }
        return false;
    }

    /** Returns true if this node contains an Audio Input node */
    bool hasAudioInputNode() const { return hasChildNode ("Internal", "audio.input"); }

    /** Returns true if this node contains an Audio Output node */
    bool hasAudioOutputNode() const { return hasChildNode ("Internal", "audio.output"); }

    /** Returns true if this node contains a MIDI Input node */
    bool hasMidiInputNode() const { return hasChildNode ("Internal", "midi.input"); }

    /** Returns true if this node contains a MIDI Output node */
    bool hasMidiOutputNode() const { return hasChildNode ("Internal", "midi.output"); }

    /** Fill a plugin Description for loading with the plugin manager */
    void getPluginDescription (PluginDescription&) const;

    //=========================================================================
    /** Write the contents of this node to file */
    bool writeToFile (const File& file) const;

    /** Save this node as a preset to file */
    bool savePresetTo (const DataPath& path, const String& name) const;

    /** Get an array of possible sources that can connect to this Node */
    void getPossibleSources (NodeArray& nodes) const;

    /** Get an array of possible destinations this node can connect to */
    void getPossibleDestinations (NodeArray& nodes) const;

    /** Returns a child node by Node ID */
    Node getNodeById (const uint32 nodeId) const;

    /** Returns a child node by UUID */
    Node getNodeByUuid (const Uuid& uuid, const bool recursive = true) const;

    //=========================================================================
    /** Rebuild this node's ports based on it's NodeObject object */
    void resetPorts();

    /** Get a port by index */
    Port getPort (const int index) const;

    /** Returns true if the passed in nodes and ports cann connect to one another */
    bool canConnect (const uint32 sourceNode, const uint32 sourcePort, const uint32 destNode, const uint32 destPort) const;

    //=========================================================================
    /** Saves the node state from NodeObject to state property */
    void savePluginState();

    /** Reads state property and applies to NodeObject */
    void restorePluginState();

    //=========================================================================
    /** Get the number of factory presets */
    int getNumPrograms() const;

    /** Change a factory preset's name */
    String getProgramName (const int index) const;

    /** Load a factory preset */
    void setCurrentProgram (const int index);

    /** Get the currently loaded factory preset */
    int getCurrentProgram() const;

    //=========================================================================
    /** True if global MIDI programs should be loaded/saved */
    bool useGlobalMidiPrograms() const;

    /** Change whether to load/save global programs */
    void setUseGlobalMidiPrograms (bool);

    /** True if MIDI program functionality is on */
    bool areMidiProgramsEnabled() const;

    /** Turn MIDI Programs on or off */
    void setMidiProgramsEnabled (bool);

    /** Returns the current MIDI program */
    int getMidiProgram() const;

    /** Sets the current midi program */
    void setMidiProgram (int program);

    /** Returns the name of a MIDI program */
    String getMidiProgramName (int program) const;

    /** Change a MIDI program's name */
    void setMidiProgramName (int program, const String& name);

    //=========================================================================
    ValueTree getArcsValueTree() const { return objectData.getChildWithName (tags::arcs); }
    ValueTree getNodesValueTree() const { return objectData.getChildWithName (tags::nodes); }
    ValueTree getParentArcsNode() const;
    ValueTree getPortsValueTree() const { return objectData.getChildWithName (tags::ports); }
    ValueTree getUIValueTree() const { return objectData.getChildWithName (tags::ui); }
    ValueTree getBlockValueTree() const noexcept { return getUIValueTree().getChildWithName (tags::block); }
    ValueTree getScriptsValueTree() const noexcept { return objectData.getChildWithName (tags::scripts); }

    //=========================================================================
    const bool operator== (const Node& o) const { return this->objectData == o.objectData; }
    const bool operator!= (const Node& o) const { return this->objectData != o.objectData; }

    /** Iterate over all ValueTree's recursively */
    void forEach (std::function<void (const ValueTree& tree)>) const;

    /** Change the block color */
    void setColor (const juce::Colour& color)
    {
        getUIValueTree().setProperty ("color", color.toString(), nullptr);
    }

    /** Returns the color of this node. */
    juce::Colour getColor() const noexcept
    {
        return juce::Colour::fromString (getUIValueTree().getProperty ("color").toString());
    }

    /** Returns the color of this node or a fallback */
    juce::Colour getColor (juce::Colour fallback) const noexcept
    {
        return getUIValueTree().hasProperty ("color") ? getColor() : fallback;
    }

    /** Add a script to this node.
        If failure, the returned will be invalid;
    */
    ValueTree addScript (const Script& script);

private:
    void setMissingProperties();
    void forEach (const ValueTree tree, std::function<void (const ValueTree& tree)>) const;
};

typedef Node NodeModel;

class NodeObjectSync final : private ValueTree::Listener {
public:
    NodeObjectSync();
    NodeObjectSync (const Node& node);
    ~NodeObjectSync();

    void setNode (const Node&);
    Node getNode() const { return node; }
    void setFrozen (bool freeze) { frozen = freeze; }
    bool isFrozen() const { return frozen; }

private:
    Node node;
    ValueTree data;
    bool frozen = false;

    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property) override;
    void valueTreeChildAdded (ValueTree& parent, ValueTree& child) override;
    void valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int index) override;
    void valueTreeChildOrderChanged (ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreeParentChanged (ValueTree& tree) override;
    void valueTreeRedirected (ValueTree& tree) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeObjectSync)
};

class PortArray : public Array<Port> {
public:
    PortArray() {}
    ~PortArray() {}
};

class NodeArray : public Array<Node> {
public:
    NodeArray() {}
    ~NodeArray() {}
    void sortByName();
};

struct ConnectionBuilder {
    ConnectionBuilder (const ConnectionBuilder& o)
    {
        operator= (o);
    }

    ConnectionBuilder& operator= (const ConnectionBuilder& o)
    {
        this->arcs = o.arcs;
        this->target = o.target;
        this->lastError = o.lastError;
        this->portChannelMap.addCopiesOf (
            o.portChannelMap, 0, o.portChannelMap.size());
        return *this;
    }

    ConnectionBuilder() : arcs (tags::arcs) {}
    ConnectionBuilder (const Node& tgt)
        : arcs (tags::arcs), target (tgt)
    {
    }

    void setTarget (const Node& newTarget)
    {
        target = newTarget;
    }

    /** Add a port that will be connected to the target's channel
        of the corresponding port type
        */
    ConnectionBuilder& addChannel (const Node& node, PortType t, const int sc, const int tc, bool input)
    {
        portChannelMap.add (new ConnectionMap (node, t, sc, tc, input));
        return *this;
    }

    void connectStereo (const Node& src, const Node& dst, int srcOffset = 0, int dstOffset = 0)
    {
        if (srcOffset < 0)
            srcOffset = 0;
        if (dstOffset < 0)
            dstOffset = 0;

        for (int i = 0; i < 2; ++i) {
            ValueTree connection (tags::arc);
            connection.setProperty (tags::sourceNode, (int64) src.getNodeId(), 0)
                .setProperty (tags::destNode, (int64) dst.getNodeId(), 0)
                .setProperty (tags::sourceChannel, i + srcOffset, 0)
                .setProperty (tags::destChannel, i + dstOffset, 0);
            arcs.addChild (connection, -1, 0);
        }
    }

    void addConnections (GraphManager& controller, const uint32 targetNodeId) const;

    String getError() const { return lastError; }

private:
    ValueTree arcs;
    Node target;
    mutable String lastError;

    struct ConnectionMap {
        ConnectionMap (const Node& node, PortType t, const int nc, const int tc, const bool input)
            : nodeId (node.getNodeId()),
              type (t),
              isInput (input),
              nodeChannel (nc),
              targetChannel (tc)
        {
        }

        const uint32 nodeId;
        const PortType type;
        const bool isInput;
        const int nodeChannel;
        const int targetChannel;
    };

    OwnedArray<ConnectionMap> portChannelMap;
};

} // namespace element
