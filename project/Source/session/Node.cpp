
#include "session/Node.h"

namespace Element {
    struct NameSorter
    {
        NameSorter() { }
        int compareElements (const Node& lhs, const Node& rhs) {
            return lhs.getName() < rhs.getName() ? -1 :
            lhs.getName() == rhs.getName() ? 0 : 1;
        }
        friend class NodeArray;
    };
    
    static void readPluginDescriptionForLoading (const ValueTree& p, PluginDescription& pd)
    {
        pd.pluginFormatName = p.getProperty (Tags::format);
        pd.fileOrIdentifier = p.getProperty (Tags::identifier);
        if (pd.fileOrIdentifier.isEmpty())
            pd.fileOrIdentifier = p.getProperty (Slugs::file);
    }
    
    
    int Port::getChannel() const
    {
        const Node node (objectData.getParent().getParent());
        if (auto* g = node.getGraphNode())
            return g->getChannelPort (getIndex());
        return -1;
    }
    
    bool Node::isProbablyGraphNode (const ValueTree& data)
    {
        return data.hasType (Tags::node) &&
            Tags::graph.toString() == data.getProperty(Tags::type).toString();
    }
    
    ValueTree Node::parse (const File& file)
    {
        ValueTree data;
        
        if (ScopedPointer<XmlElement> e = XmlDocument::parse(file))
        {
            ValueTree xml = ValueTree::fromXml (*e);
            if (xml.hasType (Tags::node))
                data = xml;
        }
        else
        {
            FileInputStream input (file);
            data = ValueTree::readFromStream (input);
        }
        
        if (data.isValid() && data.hasType (Tags::node))
        {
            Node::sanitizeProperties (data);
            return data;
        }
        
        return ValueTree();
    }
    
    bool Node::writeToFile (const File& targetFile) const
    {
        
        ValueTree data = objectData.createCopy();
        sanitizeProperties (data, true);
        
       #if EL_SAVE_BINARY_FORMAT
        FileOutputStream stream (targetFile);
        data.writeToStream (stream);
        return true;
       #else
        if (ScopedPointer<XmlElement> e = data.createXml())
            return e->writeToFile (targetFile, String());
       #endif
        return false;
    }
    
    Node Node::createGraph()
    {
        Node node (Tags::graph);
        ValueTree root = node.getValueTree();
        root.setProperty (Tags::name, "Graph 1", nullptr);
        root.getOrCreateChildWithName (Tags::nodes, nullptr);
        root.getOrCreateChildWithName (Tags::arcs, nullptr);
        return node;
    }

    ValueTree Node::makeArc (const Arc& arc)
    {
        ValueTree model (Tags::arc);
        model.setProperty (Tags::sourceNode, (int) arc.sourceNode, nullptr);
        model.setProperty (Tags::sourcePort, (int) arc.sourcePort, nullptr);
        model.setProperty (Tags::destNode,   (int) arc.destNode, nullptr);
        model.setProperty (Tags::destPort,   (int) arc.destPort, nullptr);
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
        if (arcs.hasType(Tags::nodes))
            arcs = arcs.getParent();
        if (! arcs.isValid())
            return ValueTree::invalid;
        
        jassert (arcs.hasType (Tags::node));
        return arcs.getOrCreateChildWithName (Tags::arcs, nullptr);
    }
    
    void Node::getPluginDescription (PluginDescription& p) const
    {
        readPluginDescriptionForLoading (objectData, p);
    }
    
    void Node::setMissingProperties()
    {
        stabilizePropertyString (Tags::type, "default");
        stabilizePropertyString (Tags::name, "Default Node");
        stabilizeProperty (Tags::bypass, false);
        objectData.getOrCreateChildWithName (Tags::nodes, nullptr);
    }

    GraphNode* Node::getGraphNode() const
    {
        return dynamic_cast<GraphNode*> (objectData.getProperty (Tags::object, var::null).getObject());
    }
    
    GraphNode* Node::getGraphNodeForId (const uint32 nodeId) const
    {
        const Node node (getNodeById (nodeId));
        return node.isValid() ? node.getGraphNode() : nullptr;
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
    
    void Node::getAudioInputs (PortArray& ports) const {
        getPorts (ports, PortType::Audio, true);
    }
    
    void Node::getAudioOutputs (PortArray& ports) const {
        getPorts (ports, PortType::Audio, false);
    }
    
    void Node::resetPorts()
    {
        if (GraphNodePtr ptr = getGraphNode())
        {
            ptr->resetPorts();
            ValueTree newPorts = ptr->getMetadata().getChildWithName(Tags::ports).createCopy();
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
        Arc arc ((uint32)(int) data.getProperty (Tags::sourceNode, (int) KV_INVALID_NODE),
                 (uint32)(int) data.getProperty (Tags::sourcePort, (int) KV_INVALID_PORT),
                 (uint32)(int) data.getProperty (Tags::destNode, (int) KV_INVALID_NODE),
                 (uint32)(int) data.getProperty (Tags::destPort, (int) KV_INVALID_PORT));
        return arc;
    }
    
    int Node::getNumConnections() const     { return getArcsValueTree().getNumChildren(); }
    ValueTree Node::getConnectionValueTree (const int index) const { return getArcsValueTree().getChild (index);  }
    
    void NodeArray::sortByName()
    {
        NameSorter sorter;
        this->sort (sorter);
    }
    
    bool Node::connectionExists (const ValueTree& arcs,
                                 const uint32 sourceNode, const uint32 sourcePort,
                                 const uint32 destNode, const uint32 destPort)
    {
        for (int i = arcs.getNumChildren(); --i >= 0;)
        {
            const ValueTree arc (arcs.getChild (i));
            if (static_cast<int> (sourceNode) == (int) arc.getProperty (Tags::sourceNode) &&
                static_cast<int> (sourcePort) == (int) arc.getProperty (Tags::sourcePort) &&
                static_cast<int> (destNode) == (int) arc.getProperty (Tags::destNode) &&
                static_cast<int> (destPort) == (int) arc.getProperty (Tags::destPort))
            {
                return true;
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
    
    Port Node::getPort (const int index) const
    {
        Port port (getPortsValueTree().getChildWithProperty (Tags::index, index));
        return port;
    }
    
    bool Node::canConnect (const uint32 sourceNode, const uint32 sourcePort,
                           const uint32 destNode, const uint32 destPort) const
    {
        const Node sn (getNodeById (sourceNode));
        const Node dn (getNodeById (destNode));
        if (!sn.isValid() || !dn.isValid())
            return false;
        
        const Port dp (dn.getPort ((int) destPort));
        const Port sp (sn.getPort ((int) sourcePort));
        return sp.getType().canConnect (dp.getType());
    }
    
    void Node::setRelativePosition (const double x, const double y)
    {
        setProperty ("relativeX", x);
        setProperty ("relativeY", y);
    }

    void Node::getRelativePosition (double& x, double& y) const
    {
        x = (double) getProperty ("relativeX", 0.5f);
        y = (double) getProperty ("relativeY", 0.5f);
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
    
    void Node::savePluginState()
    {
        if (! isValid())
            return;
        
        MemoryBlock state;
        if (GraphNodePtr obj = getGraphNode())
            if (auto* proc = obj->getAudioProcessor())
                proc->getStateInformation (state);
        if (state.getSize() > 0)
            objectData.setProperty (Tags::state, state.toBase64Encoding(), nullptr);
    }
    
    void Node::setCurrentProgram (const int index)
    {
        if (auto* obj = getGraphNode())
            obj->getAudioProcessor()->setCurrentProgram (index);
    }
    
    int Node::getCurrentProgram() const
    {
        if (auto* obj = getGraphNode())
            obj->getAudioProcessor()->getParameters();
        
        if (auto* obj = getGraphNode())
            return (const_cast<AudioProcessor*>(obj->getAudioProcessor()))->getCurrentProgram();
        
        return -1;
    }
    
    String Node::getProgramName (const int index) const
    {
        if (auto* obj = getGraphNode())
            return obj->getAudioProcessor()->getProgramName (index);
        
        return String();
    }
    
    int Node::getNumPrograms() const
    {
        if (auto* obj = getGraphNode())
            return obj->getAudioProcessor()->getNumPrograms();
        
        return 0;
    }
}
