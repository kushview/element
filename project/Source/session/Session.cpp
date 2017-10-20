/*
    Session.cpp - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#include "engine/AudioEngine.h"
#include "engine/InternalFormat.h"
#include "engine/Transport.h"
#include "session/Node.h"
#include "MediaManager.h"
#include "Globals.h"

#include "session/Session.h"

namespace Element {
    class Session::Private
    {
    public:
        Private (Session& s)
            : session (s)
        { }

        ~Private() { }
    private:
        friend class Session;
        Session&                     session;
    };

    Session::Session()
        : ObjectModel (Tags::session)
    {
        priv = new Private (*this);
        setMissingProperties (true);
        objectData.addListener (this);
    }

    Session::~Session()
    {
        objectData.removeListener (this);
        clear();

        priv = nullptr;

        if (getReferenceCount() > 0) {
            std::clog << "Session Destroyed with " << getReferenceCount() << " references still existing\n";
        } else {
            std::clog << "Session Destroyed with no references\n";
        }
    }
    
    bool Session::addGraph (const Node &node, const bool setActive)
    {
        jassert(! node.getValueTree().getParent().isValid());
        auto graphs = getGraphsValueTree();
        graphs.addChild (node.getValueTree(), -1, nullptr);
        if (setActive)
            graphs.setProperty (Tags::active, graphs.indexOf (node.getValueTree()), nullptr);
        return true;
    }
    
    Node Session::getActiveGraph() const
    {
        const int index = getActiveGraphIndex();
        if (isPositiveAndBelow (index, getNumGraphs()))
            return getGraph (index);
    
        ValueTree graphs = getGraphsValueTree();
        graphs.setProperty (Tags::active, graphs.getNumChildren() > 0 ? 0 : -1, nullptr);
        return  graphs.getNumChildren() > 0 ? getGraph(0) : Node();
    }
    
    int Session::getActiveGraphIndex() const
    {
        return getGraphsValueTree().getProperty (Tags::active, -1);
    }
    
    void Session::clear()
    {
        setMissingProperties (true);
    }

    bool Session::loadData (const ValueTree &data)
    {
        if (! data.hasType (Tags::session))
            return false;
        objectData.removeListener (this);
        objectData = data;
        objectData.addListener (this);
        return true;
    }

    XmlElement* Session::createXml()
    {
        ValueTree saveData = objectData.createCopy();
        Node::sanitizeProperties (saveData, true);
        return saveData.createXml();
    }

    void Session::setMissingProperties (bool resetExisting)
    {
        if (resetExisting)
            objectData.removeAllProperties (nullptr);

        if (! objectData.hasProperty (Tags::name))
            setProperty (Tags::name, "Untitled");
        if (! objectData.hasProperty (Slugs::tempo))
            setProperty (Tags::tempo, (double) 120.0);
        
        if (resetExisting)
            objectData.removeAllChildren (nullptr);
        
        ValueTree graphs = objectData.getOrCreateChildWithName (Tags::graphs, nullptr);
    }

    void Session::notifyChanged()
    {
        if (freezeChangeNotification)
            return;
        sendChangeMessage();
    }
    
    void Session::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
    {
        if (property == Tags::object ||
            (tree.hasType(Tags::node) && property == Tags::state))
        {
            return;
        }
        
        notifyChanged();
    }

    void Session::valueTreeChildAdded (ValueTree& parentTree, ValueTree& child)
    {
        notifyChanged();
    }

    void Session::valueTreeChildRemoved (ValueTree& parentTree, ValueTree& child, int)
    {
        notifyChanged();
    }

    void Session::valueTreeChildOrderChanged (ValueTree& parent, int, int) {  }

    void Session::valueTreeParentChanged (ValueTree& tree) { }
    void Session::valueTreeRedirected (ValueTree& tree) { }
    
    void Session::saveGraphState()
    {
        auto graphs = getGraphsValueTree();
        for (int i = 0; i < graphs.getNumChildren(); ++i)
        {
            auto graph = graphs.getChild (i);
            auto nodes = graph.getChildWithName(Tags::nodes);
            for (int j = 0; j < nodes.getNumChildren(); ++j)
            {
                Node node (nodes.getChild (j), false);
                if (GraphNodePtr ptr = node.getGraphNode())
                {
                    MemoryBlock state;
                    if (auto* proc = ptr->getAudioProcessor())
                        proc->getStateInformation (state);
                    if (state.getSize() > 0)
                        node.setProperty (Tags::state, state.toBase64Encoding());
                }
            }
        }
    }
}
