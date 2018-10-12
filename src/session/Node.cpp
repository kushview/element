
#include "session/Node.h"
#include "controllers/GraphController.h"

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
        if (p.getProperty(Tags::type) == "graph")
        {
            pd.name = p.getProperty (Tags::name);
            pd.fileOrIdentifier = "element.graph";
            pd.pluginFormatName = "Element";
        }
        else
        {
            // plugins and io nodes
            pd.pluginFormatName = p.getProperty (Tags::format);
            pd.fileOrIdentifier = p.getProperty (Tags::identifier);
            if (pd.fileOrIdentifier.isEmpty())
                pd.fileOrIdentifier = p.getProperty (Slugs::file);
        }
    }
    
    int Port::getChannel() const
    {
        const Node node (objectData.getParent().getParent());
        if (auto* g = node.getGraphNode())
            return g->getChannelPort (getIndex());
        return -1;
    }
    
    Node Node::createDefaultGraph (const String& name)
    {
        Node graph (Tags::graph);
        graph.setProperty (Tags::name, name.isNotEmpty() ? name : "Graph");
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
                  .setProperty (Tags::name, names [types.indexOf (t)], 0);

            if (t == "audio.input")
            {
                ioNode.setProperty ("relativeX", 0.25f, 0)
                      .setProperty ("relativeY", 0.25f, 0)
                      .setProperty ("numAudioIns", 0, 0)
                      .setProperty ("numAudioOuts", 2, 0);

                ValueTree port (Tags::port);
                port.setProperty ("name", "Port", 0)
                    .setProperty("index", portIdx++, 0)
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
                ioNode.setProperty ("relativeX", 0.25f, 0)
                      .setProperty ("relativeY", 0.75f, 0)
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
                ioNode.setProperty ("relativeX", 0.75f, 0)
                      .setProperty ("relativeY", 0.25f, 0)
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
                ioNode.setProperty ("relativeX", 0.75f, 0)
                      .setProperty ("relativeY", 0.75f, 0)
                      .setProperty ("numAudioIns", 0, 0)
                      .setProperty ("numAudioOuts", 0, 0);
                ValueTree port (Tags::port);
                port.setProperty ("name", "Port", 0)
                    .setProperty ("index", portIdx++, 0)
                    .setProperty ("type", "midi", 0)
                    .setProperty ("flow", "input", 0);
                ports.addChild (port, -1, 0);
            }

            nodes.addChild (ioNode, -1, 0);
        }

        return graph;
    }

    bool Node::isProbablyGraphNode (const ValueTree& data)
    {
        return data.hasType (Tags::node) &&
            Tags::graph.toString() == data.getProperty(Tags::type).toString();
    }
    
    ValueTree Node::parse (const File& file)
    {
        ValueTree data;
        ValueTree nodeData;
        
        if (ScopedPointer<XmlElement> e = XmlDocument::parse(file))
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
                nodeData.setProperty (Tags::name, data.getProperty(Tags::name), 0);
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
    
    bool Node::savePresetTo (const DataPath& path, const String& name) const
    {
        {
            // hack: ensure the plugin's state info is up-to-date
            Node(*this).savePluginState();
        }
        
        ValueTree preset (Tags::preset);
        ValueTree data = objectData.createCopy();
        sanitizeProperties (data, true);
        preset.addChild (data, -1, 0);
        
        const auto targetFile = path.createNewPresetFile (*this, name);
        data.setProperty (Tags::name, targetFile.getFileNameWithoutExtension(), 0);
        data.setProperty (Tags::type, Tags::node.toString(), 0);
        
       #if EL_SAVE_BINARY_FORMAT
        FileOutputStream stream (targetFile);
        preset.writeToStream (stream);
        return true;
       #else
        if (ScopedPointer<XmlElement> e = preset.createXml())
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
            return ValueTree();
        
        jassert (arcs.hasType (Tags::node));
        return arcs.getOrCreateChildWithName (Tags::arcs, nullptr);
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
        objectData.getOrCreateChildWithName (Tags::nodes, nullptr);
        objectData.getOrCreateChildWithName (Tags::ports, nullptr);
    }

    GraphNode* Node::getGraphNode() const
    {
        return dynamic_cast<GraphNode*> (objectData.getProperty (Tags::object, var()).getObject());
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
            // workaround to keep IO node names correct. note that
            // setParentGraph may or may not call reset ports
            if (auto* parent = ptr->getParentGraph()) {
                ptr->setParentGraph (parent);
                if (ptr->isMidiIONode() || ptr->isAudioIONode())
                    setProperty (Tags::name, ptr->getAudioProcessor()->getName());
            }
            
            // if (isAudioIONode() || isMidiIONode())
            // {
            //     // need to keep IONode ports
            // }
            // else
            {
                ptr->resetPorts();
                ValueTree newPorts = ptr->getMetadata().getChildWithName(Tags::ports).createCopy();
                ValueTree ports = getPortsValueTree();
                objectData.removeChild (ports, nullptr);
                objectData.addChild (newPorts, -1, nullptr);
            }
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
    
    void Node::restorePluginState()
    {
        if (! isValid() || !objectData.hasProperty (Tags::state))
            return;
        
        if (GraphNodePtr obj = getGraphNode())
        {
            if (auto* const proc = obj->getAudioProcessor())
            {
                proc->setCurrentProgram ((int) objectData.getProperty (Tags::program, 0));

                auto data = getProperty(Tags::state).toString().trim();
                if (data.isNotEmpty())
                {
                    MemoryBlock state; state.fromBase64Encoding (data);
                    if (state.getSize() > 0)
                    {
                        proc->setStateInformation (state.getData(), (int)state.getSize());
                    }
                }
			    
				data = getProperty(Tags::programState).toString().trim();
				if (data.isNotEmpty())
				{
					MemoryBlock state; state.fromBase64Encoding (data);
					if (state.getSize() > 0)
					{
						proc->setCurrentProgramStateInformation(state.getData(), 
							(int)state.getSize());
					}
				}

                proc->suspendProcessing (isBypassed());
            }
        }

        // this was originally here to help reduce memory usage
        // need another way to free this property without disturbing
        // the normal flow of the app.
        const bool clearStateProperty = false;
        if (clearStateProperty)
            objectData.removeProperty (Tags::state, 0);

        for (int i = 0; i < getNumNodes(); ++i)
            getNode(i).restorePluginState();
    }
    
    void Node::savePluginState()
    {
        if (! isValid())
            return;
        
        if (GraphNodePtr obj = getGraphNode())
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
        }

        for (int i = 0; i < getNumNodes(); ++i)
            getNode(i).savePluginState();
    }
    
    void Node::setCurrentProgram (const int index)
    {
        if (auto* obj = getGraphNode())
            obj->getAudioProcessor()->setCurrentProgram (index);
    }
    
    int Node::getCurrentProgram() const
    {
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
            if (auto* proc = obj->getAudioProcessor())
                return proc->getNumPrograms();
        return 0;
    }

    bool Node::hasEditor() const
    {
        if (Tags::plugin == getNodeType())
            if (auto gn = getGraphNode())
                if (auto* const proc = getGraphNode()->getAudioProcessor())
                    return proc->hasEditor();
        return false;
    }

    void ConnectionBuilder::addConnections (GraphController& controller, const uint32 targetNodeId) const
    {
        GraphNodePtr tgt = controller.getNodeForId (targetNodeId);
        if (tgt)
        {
            bool anythingAdded = false;
            for (const auto* pc : portChannelMap)
            {
                GraphNodePtr ptr = controller.getNodeForId (pc->nodeId);
                if (! ptr)
                    continue;

                if (pc->isInput)
                {
                    anythingAdded |= controller.addConnection (
                        tgt->nodeId, tgt->getPortForChannel (pc->type, pc->targetChannel, ! pc->isInput),
                        ptr->nodeId, ptr->getPortForChannel (pc->type, pc->nodeChannel, pc->isInput)
                    );
                }
                else
                {
                    anythingAdded |= controller.addConnection (
                        ptr->nodeId, ptr->getPortForChannel (pc->type, pc->nodeChannel, pc->isInput),
                        tgt->nodeId, tgt->getPortForChannel (pc->type, pc->targetChannel, ! pc->isInput)
                    );
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
}
