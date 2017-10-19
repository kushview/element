/*
    Node.h - This file is part of Element
    Copyright (C) 2014-2017  Kushview, LLC.  All rights reserved.
 */

#pragma once

#include "ElementApp.h"
#include "engine/GraphNode.h"

namespace Element {
    
    class NodeArray;
    class PortArray;
    
    class Port : public ObjectModel
    {
    public:
        Port() : ObjectModel (Tags::port) { }
        Port (const ValueTree& p) : ObjectModel (p) { jassert (p.hasType (Tags::port)); }
        ~Port() { }
        
        int getChannel() const { return 0; }
        
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
        
        const String getName() const { return getProperty (Slugs::name, "Port"); }
        
        const PortType getType() const {
            return PortType (getProperty (Slugs::type, "unknown"));
        }
        
        bool isA (const PortType type, const bool isInputFlow) const {
            return getType() == type && isInputFlow == isInput();
        }
        
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
            jassert (data.hasType (Tags::node));
            if (setMissing)
                setMissingProperties();
        }
        
        Node (const Identifier& nodeType)
            : ObjectModel (Tags::node)
        {
            objectData.setProperty (Slugs::type, nodeType.toString(), nullptr);
            setMissingProperties();
        }
        
        /** Creates an empty graph model */
        static Node createGraph();
        
        /** Returns true if the value tree is probably a graph node */
        static bool isProbablyGraphNode (const ValueTree& data);

        /** Load a node from file */
        static ValueTree parse (const File& file);
        
        /** Removes properties that can't be saved to a file. e.g. object properties */
        static void sanitizeProperties (ValueTree node, const bool recursive = false)
        {
            node.removeProperty (Tags::object, nullptr);
            if (recursive)
                for (int i = 0; i < node.getNumChildren(); ++i)
                    sanitizeProperties (node.getChild(i), recursive);
        }
        
        /** Create a value tree version of an arc */
        static ValueTree makeArc (const kv::Arc& arc);

        /** Returns true if the underlying data is probably a node */
        bool isValid() const { return objectData.hasType (Tags::node); }
        
        /** Returns true if this node is probably a graph */
        bool isGraph() const { return isProbablyGraphNode (objectData); }
        
        /** Returns the nodeId as defined in the engine */
        const uint32 getNodeId() const { return (uint32)(int64) getProperty (Tags::id); }

        /** Returns an Identifier indicating this nodes type */
        const Identifier getNodeType() const { return Identifier (getProperty(Slugs::type).toString()); }

        const bool hasNodeType (const Identifier& t) const { return getNodeType() == t; }
        
        const String getName() const { return getProperty (Slugs::name); }
        
        GraphNode* getGraphNode() const;
        const int getNumAudioIns()  const { return (int) getProperty ("numAudioIns", 0); }
        const int getNumAudioOuts() const { return (int) getProperty ("numAudioOuts", 0); }
        
        const bool canConnectTo (const Node& o) const;

        void getPorts (PortArray& ports, PortType type, bool isInput) const;
        void getPorts (PortArray& ins, PortArray& outs, PortType type) const;
        void getAudioInputs (PortArray& ports) const;
        void getAudioOutputs (PortArray& ports) const;
        
        void getPluginDescription (PluginDescription&) const;
        
        /** Write the contents of this node to file */
        bool writeToFile (const File& file) const;
    
        ValueTree getArcsValueTree()  const { return objectData.getChildWithName (Tags::arcs); }
        ValueTree getNodesValueTree() const { return objectData.getChildWithName (Tags::nodes); }
        ValueTree getParentArcsNode() const;
        ValueTree getPortsValueTree() const { return objectData.getChildWithName (Tags::ports); }

    private:
        void setMissingProperties();
    };
    
    typedef Node NodeModel;
    
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
    
}
