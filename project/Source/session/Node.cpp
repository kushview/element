
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
        objectData.getOrCreateChildWithName(Tags::nodes, nullptr);
        stabilizePropertyString (Slugs::type, "default");
        stabilizePropertyString (Slugs::name, "Default Node");
    }

    GraphNode* Node::getGraphNode() const
    {
        return dynamic_cast<GraphNode*> (objectData.getProperty (Tags::object, var::null).getObject());
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
    
    void NodeArray::sortByName()
    {
        NameSorter sorter;
        this->sort (sorter);
    }
}
