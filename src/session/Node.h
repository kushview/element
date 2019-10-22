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

#pragma once

#include "ElementApp.h"
#include "engine/GraphNode.h"

namespace Element {

class GraphManager;
class NodeArray;
class PortArray;

class Port : public ObjectModel
{
public:
    Port() : ObjectModel (Tags::port) { }
    Port (const ValueTree& p) : ObjectModel (p) { jassert (p.hasType (Tags::port)); }
    ~Port() { }
    
    /** Returns the ValueTree of the Node containing this port
        will not always be valid 
        */
    inline ValueTree getNode() const { return objectData.getParent().getParent(); }

    /** Returns true if this port probably lives on a Node */
    inline bool hasParentNode() const { return getNode().hasType (Tags::node); }

    /** Returns the coresponding channel for this port's index */
    int getChannel() const;
    
    const bool isInput() const
    {
        jassert (objectData.hasProperty ("flow"));
        return getProperty("flow", "").toString() == "input";
    }
    
    const bool isOutput() const
    {
        jassert (objectData.hasProperty ("flow"));
        return getProperty("flow", "").toString() == "output";
    }
    
    const String getName() const { return getProperty (Tags::name, "Port"); }
    const PortType getType() const { return PortType (getProperty (Slugs::type, "unknown").toString()); }
    bool isA (const PortType type, const bool isInputFlow) const { return getType() == type && isInputFlow == isInput(); }
    
    uint32 getIndex() const
    {
        const int index = getProperty (Slugs::index, -1);
        return index >= 0 ? static_cast<uint32> (index) : KV_INVALID_PORT;
    }

    operator uint32() const { return getIndex(); }
};

class Node : public ObjectModel
{
public:
    /** Create an invalid node */
    Node() : ObjectModel() { }
    
    Node (const ValueTree& data, const bool setMissing = true)
        : ObjectModel (data)
    {
        if (setMissing)
        {
            jassert (data.hasType (Tags::node));
            setMissingProperties();
        }
        else
        {
            // noop
        }
    }
    
    Node (const Identifier& nodeType)
        : ObjectModel (Tags::node)
    {
        objectData.setProperty (Slugs::type, nodeType.toString(), nullptr);
        setMissingProperties();
    }
    
    ~Node() noexcept { }
    
    /** Returns true if the connection exists in the provided ValueTree
     
        @param checkMissing  If true, will return false if found but has the missing property
        */
    static bool connectionExists (const ValueTree& arcs, const uint32 sourceNode, const uint32 sourcePort,
                                    const uint32 destNode, const uint32 destPort, const bool checkMissing = false);
    
    static Node createDefaultGraph (const String& name = String());

    /** Creates an empty graph model */
    static Node createGraph (const String& name = String());
    
    /** Returns true if the value tree is probably a graph node */
    static bool isProbablyGraphNode (const ValueTree& data);

    /** Returns true if this node should be enabled */
    inline const bool isEnabled() const { return (bool) getProperty (Tags::enabled, true); }

    static ValueTree resetIds (const ValueTree& data);
    
    /** Load a node from file */
    static ValueTree parse (const File& file);
    
    /** Removes properties that can't be saved to a file. e.g. object properties */
    static void sanitizeProperties (ValueTree node, const bool recursive = false)
    {
        node.removeProperty (Tags::object, nullptr);
        
        if (node.hasType (Tags::node))
        {
            Array<Identifier> properties ({ Tags::offline, 
                Tags::placeholder, Tags::missing });
            for (const auto& property : properties)
                node.removeProperty (property, nullptr);
        }

        if (recursive)
            for (int i = 0; i < node.getNumChildren(); ++i)
                sanitizeProperties (node.getChild(i), recursive);
    }
    
    /** this is just an alias right now */
    static void sanitizeRuntimeProperties (ValueTree node, const bool recursive = false) {
        sanitizeProperties (node, recursive);
    }

    /** Create a value tree version of an arc */
    static ValueTree makeArc (const kv::Arc& arc);

    /** Create an Arc from a ValueTree */
    static Arc arcFromValueTree (const ValueTree& data);

    Node getParentGraph() const;
    bool isChildOfRootGraph() const;
    String getUuidString() const { return objectData.getProperty(Tags::uuid).toString(); }
    Uuid getUuid() const { return Uuid (getUuidString()); }
    
    inline kv::MidiChannels getMidiChannels() const
    {
        kv::MidiChannels chans;
       #ifndef EL_FREE
        if (objectData.hasProperty (Tags::midiChannels))
        {
            if (auto* const block = objectData.getProperty(Tags::midiChannels).getBinaryData())
            {
                BigInteger data; data.loadFromMemoryBlock (*block);
                chans.setChannels (data);
            }
        }
        else
       #endif
        {
            const auto channel = (int) objectData.getProperty (Tags::midiChannel, 0);
            if (channel > 0)
                chans.setChannel (channel);
            else
                chans.setOmni (true);
        }
        return chans;
    }

    bool isBypassed() const { return objectData.getProperty(Tags::bypass, false); }
    Value getBypassedValue() { return getPropertyAsValue (Tags::bypass); }
    
    bool isMuted() const { return (bool) getProperty (Tags::mute, false); }
    Value getMutedValue() { return getPropertyAsValue (Tags::mute); }
    bool isMutingInputs() const { return (bool) getProperty ("muteInput", false); }
    void setMuted (bool);
    void setMuteInput (bool);

    /** returns the number of connections on this node */
    int getNumConnections() const;
    ValueTree getConnectionValueTree (const int index) const;
    
    /** Returns true if the underlying data is probably a node */
    bool isValid() const { return objectData.hasType (Tags::node); }
    
    /** Returns true if this node is probably a graph */
    bool isGraph() const { return isProbablyGraphNode (objectData); }
    
    /** Returns the nodeId as defined in the engine */
    const uint32 getNodeId() const { return (uint32)(int64) getProperty (Tags::id); }

    /** Returns an Identifier indicating this nodes type */
    const Identifier getNodeType() const
    {
        auto type = getProperty(Slugs::type).toString();
        return type.isNotEmpty() ? type : Identifier ("unknown");
    }

    /** Set relative position */
    void setRelativePosition (const double x, const double y);
    void getRelativePosition (double& x, double& y) const;
    
    const bool hasNodeType (const Identifier& t) const { return getNodeType() == t; }

    
    /** Returns the user-modifiable name of this node */
    const String getName() const { return getProperty (Tags::name); }

    /** Returns the node name defined by the user. If not set it returns the
        node name set when loaded. 
     */
    const String getDisplayName() const;

    const String getPluginName() const;

    bool hasModifiedName() const
    {
        auto dname = getName();
        return dname.isNotEmpty() && dname != getPluginName();
    }

    GraphNode* getGraphNode() const;
    
    int getNumNodes() const         { return getNodesValueTree().getNumChildren(); }
    Node getNode (const int index)  const { return Node (getNodesValueTree().getChild(index), false); }
    
    /** Returns a child graph node object by id */
    GraphNode* getGraphNodeForId (const uint32) const;
    
    const int getNumAudioIns()  const { return (int) getProperty ("numAudioIns", 0); }
    const int getNumAudioOuts() const { return (int) getProperty ("numAudioOuts", 0); }
    
    const bool canConnectTo (const Node& o) const;

    int getNumPorts() const { return getPortsValueTree().getNumChildren(); }
    void getPorts (PortArray& ports, PortType type, bool isInput) const;
    void getPorts (PortArray& ins, PortArray& outs, PortType type) const;
    void getAudioInputs (PortArray& ports) const;
    void getAudioOutputs (PortArray& ports) const;
    
    inline bool isAudioIONode() const
    {
        return objectData.getProperty(Tags::format) == "Internal" &&
            (objectData.getProperty(Tags::identifier) == "audio.input" ||
                objectData.getProperty(Tags::identifier) == "audio.output");
    }
    
    inline bool isAudioInputNode() const
    {
        return objectData.getProperty(Tags::format) == "Internal" &&
            objectData.getProperty(Tags::identifier) == "audio.input";
    }

    inline bool isAudioOutputNode() const
    {
        return objectData.getProperty(Tags::format) == "Internal" &&
            objectData.getProperty(Tags::identifier) == "audio.output";
    }

    inline bool isMidiIONode() const
    {
        return objectData.getProperty(Tags::format) == "Internal" &&
            (objectData.getProperty(Tags::identifier) == "midi.input" ||
                objectData.getProperty(Tags::identifier) == "midi.output");
    }
    
    inline bool isMidiInputNode() const
    {
        return objectData.getProperty(Tags::format) == "Internal" &&
            objectData.getProperty(Tags::identifier) == "midi.input";
    }

    inline bool isMidiOutputNode() const
    {
        return objectData.getProperty(Tags::format) == "Internal" &&
            objectData.getProperty(Tags::identifier) == "midi.output";
    }

    /** Returns the format of this node */
    inline const var& getFormat() const        { return objectData.getProperty(Tags::format); }

    /** Returns this nodes identifier */
    inline const var& getIdentifier() const    { return objectData.getProperty(Tags::identifier); }

    /** Returns a file property if exists, otherwise the identifier property */
    inline const var& getFileOrIdentifier() const
    { 
        return objectData.hasProperty(Tags::file) ? objectData.getProperty (Tags::file)
                                                    : getIdentifier();
    }

    inline bool isIONode() const { return isAudioIONode() || isMidiIONode(); }
    
    /** returns the first node by format and identifier */
    inline Node getNodeByFormat (const var& format, const var& identifier) const
    {
        auto nodes = getNodesValueTree();

        for (int i = 0; i < nodes.getNumChildren(); ++i)
        {
            auto child = nodes.getChild (i);
            if (child[Tags::format] == format && child[Tags::identifier] == identifier)
                return Node (child, false);
        }

        return Node();
    }

    Node getIONode (PortType portType, const bool isInput) const
    {
        if (!portType.isAudio() && !portType.isMidi())
            return Node();
        
        String identifier = portType.getSlug();
        identifier << "." << String((isInput) ? "input" : "output");
        return getNodeByFormat ("Internal", identifier);
    }

    bool hasChildNode (const var& format, const var& identifier) const
    {
        auto nodes = getNodesValueTree();
        for (int i = 0; i < nodes.getNumChildren(); ++i)
        {
            auto child = nodes.getChild(i);
            if (child[Tags::format] == format && child[Tags::identifier] == identifier)
                return true;
        }
        return false;
    }
    
    bool hasAudioInputNode() const      { return hasChildNode ("Internal", "audio.input"); }
    bool hasAudioOutputNode() const     { return hasChildNode ("Internal", "audio.output"); }
    bool hasMidiInputNode() const       { return hasChildNode ("Internal", "midi.input"); }
    bool hasMidiOutputNode() const      { return hasChildNode ("Internal", "midi.output"); }
    
    /** Fill a plugin Description for loading with the plugin manager */
    void getPluginDescription (PluginDescription&) const;
    
    /** Write the contents of this node to file */
    bool writeToFile (const File& file) const;

    bool savePresetTo (const DataPath& path, const String& name) const;
    
    /** Returns true if this is a root graph on the session */
    bool isRootGraph() const
    {
        return objectData.getParent().hasType (Tags::graphs) &&
                objectData.getParent().getParent().hasType (Tags::session);
    }
    
    void getPossibleSources (NodeArray& nodes) const;
    void getPossibleDestinations (NodeArray& nodes) const;
    
    Node getNodeById (const uint32 nodeId) const;
    Node getNodeByUuid (const Uuid& uuid, const bool recursive = true) const;

    void resetPorts();
    Port getPort (const int index) const;
    bool canConnect (const uint32 sourceNode, const uint32 sourcePort,
                        const uint32 destNode, const uint32 destPort) const;
    
    /** Saves the node state from GraphNode to state property */
    void savePluginState();
    
    /** Reads state property and applies to GraphNode */
    void restorePluginState();
    
    int getNumPrograms() const;
    String getProgramName (const int index) const;
    void setCurrentProgram (const int index);
    int getCurrentProgram() const;
    
    /** True if global MIDI programs should be loaded/saved */
    bool useGlobalMidiPrograms() const;
    /** Change whether to load/save global programs */
    void setUseGlobalMidiPrograms (bool);
    /** True if MIDI program functionality is on */
    bool areMidiProgramsEnabled() const;
    /** Turn MIDI Programs on or off */
    void setMidiProgramsEnabled (bool);
    
    int getMidiProgram() const;
    void setMidiProgram (int program);
    String getMidiProgramName (int program) const;
    void setMidiProgramName (int program, const String& name);

    bool hasEditor() const;
    
    void getArcs (OwnedArray<Arc>&) const;

    ValueTree getArcsValueTree()  const { return objectData.getChildWithName (Tags::arcs); }
    ValueTree getNodesValueTree() const { return objectData.getChildWithName (Tags::nodes); }
    ValueTree getParentArcsNode() const;
    ValueTree getPortsValueTree() const { return objectData.getChildWithName (Tags::ports); }
    ValueTree getUIValueTree()    const { return objectData.getChildWithName (Tags::ui); }
    const bool operator==(const Node& o) const { return this->objectData == o.objectData; }
    const bool operator!=(const Node& o) const { return this->objectData != o.objectData; }

    void forEach (std::function<void(const ValueTree& tree)>) const;

private:
    void setMissingProperties();
    void forEach (const ValueTree tree, std::function<void(const ValueTree& tree)>) const;
};

typedef Node NodeModel;

class NodeObjectSync final : private ValueTree::Listener
{
public:
    NodeObjectSync();
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
};

class PortArray : public Array<Port>
{
public:
    PortArray() { }
    ~PortArray() { }
};

class NodeArray : public Array<Node>
{
public:
    NodeArray() { }
    ~NodeArray() { }
    void sortByName();
};

struct ConnectionBuilder
{
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

    ConnectionBuilder() : arcs (Tags::arcs) { }
    ConnectionBuilder (const Node& tgt)
        : arcs (Tags::arcs),  target (tgt)
    { }

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

    void connectStereo (const Node& src, const Node& dst,
                        int srcOffset = 0, int dstOffset = 0)
    {
        if (srcOffset < 0) srcOffset = 0;
        if (dstOffset < 0) dstOffset = 0;

        for (int i = 0; i < 2; ++i)
        {
            ValueTree connection (Tags::arc);
            connection.setProperty (Tags::sourceNode, (int64) src.getNodeId(), 0)
                        .setProperty (Tags::destNode, (int64) dst.getNodeId(), 0)
                        .setProperty (Tags::sourceChannel, i + srcOffset, 0)
                        .setProperty (Tags::destChannel, i + dstOffset, 0);
            arcs.addChild (connection, -1, 0);
        }
    }

    void addConnections (GraphManager& controller, const uint32 targetNodeId) const;

    String getError() const { return lastError; }

private:
    ValueTree arcs;
    Node target;
    mutable String lastError;

    struct ConnectionMap
    {
        ConnectionMap (const Node& node, PortType t, 
                        const int nc, const int tc, 
                        const bool input)
            : nodeId (node.getNodeId()),
                type (t), isInput (input),
                nodeChannel (nc), targetChannel (tc)
        { }

        const uint32 nodeId;
        const PortType type;
        const bool isInput;
        const int nodeChannel;
        const int targetChannel;
    };

    OwnedArray<ConnectionMap> portChannelMap;
};

}
