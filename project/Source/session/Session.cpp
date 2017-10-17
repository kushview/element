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
    
    bool Session::addGraph (const Element::Node &node)
    {
        jassert(! node.getValueTree().getParent().isValid());
        auto graphs = getGraphsValueTree();
        graphs.addChild (node.getValueTree(), -1, nullptr);
        return true;
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
        XmlElement* e = saveData.createXml();
        if (nullptr != e)
            polishXml (*e);

        return e;
    }

    void Session::polishXml (XmlElement &e)
    {
        // noop
    }

    void Session::setMissingProperties (bool resetExisting)
    {
        if (resetExisting)
            objectData.removeAllProperties (nullptr);
        if (! node().hasProperty (Tags::name))
            setProperty (Tags::name, "Untitled");
        if (! node().hasProperty (Slugs::tempo))
            setProperty (Tags::tempo, (double) 120.f);
        
        if (resetExisting)
            objectData.removeAllChildren (nullptr);
        
        ValueTree graphs = objectData.getOrCreateChildWithName (Tags::graphs, nullptr);
        ValueTree root = graphs.getOrCreateChildWithName (Tags::node, nullptr);
        root.setProperty (Slugs::type, Tags::graph.toString(), nullptr);
        root.setProperty (Tags::name, "Root", nullptr);
        ValueTree nodes = root.getOrCreateChildWithName (Tags::nodes, nullptr);
    }

    void Session::valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
    {
        if (property == Tags::object ||
            (tree.hasType(Tags::node) && property == Tags::state))
        {
            return;
        }
        
        sendChangeMessage();
    }

    void Session::valueTreeChildAdded (ValueTree& parentTree, ValueTree& child)
    {
        sendChangeMessage();
    }

    void Session::valueTreeChildRemoved (ValueTree& parentTree, ValueTree& child, int)
    {
         sendChangeMessage();
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
