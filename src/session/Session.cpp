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
        
        jassert (getReferenceCount() == 0);
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

        ScopedFrozenLock sfl (*this);
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
        setMissingProperties();
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
        if (! objectData.hasProperty (Tags::tempo))
            setProperty (Tags::tempo, (double) 120.0);
        if (! objectData.hasProperty (Tags::notes))
            setProperty (Tags::notes, String());
        if (! objectData.hasProperty(Tags::beatsPerBar))
            setProperty (Tags::beatsPerBar, 4);
        if (! objectData.hasProperty (Tags::beatDivisor))
            setProperty (Tags::beatDivisor, (int) BeatType::QuarterNote);
        
        if (resetExisting)
            objectData.removeAllChildren (nullptr);
        
        ValueTree graphs = objectData.getOrCreateChildWithName (Tags::graphs, nullptr);
        ValueTree controllers = objectData.getOrCreateChildWithName (Tags::controllers, nullptr);
        ignoreUnused (graphs, controllers);
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
        
        if (tree == objectData && property == Tags::tempo) {
        
        }
        notifyChanged();
    }

    void Session::valueTreeChildAdded (ValueTree& parent, ValueTree& child)
    {
        // controller device added
        if (parent.getParent() == objectData && 
            parent.hasType (Tags::controllers) && 
            child.hasType (Tags::controller))
        {
            const ControllerDevice device (child);
            controllerDeviceAdded (device);
        }

        // controller device control added
        if (parent.getParent().getParent() == objectData && 
            parent.getParent().hasType (Tags::controllers) && 
            parent.hasType (Tags::controller) && 
            child.hasType (Tags::control))
        {
            const ControllerDevice::Control control (child);
            controlAdded (control);
        }

        notifyChanged();
    }

    void Session::valueTreeChildRemoved (ValueTree& parent, ValueTree& child, int)
    {
        // controller device removed
        if (parent.getParent() == objectData && 
            parent.hasType (Tags::controllers) && 
            child.hasType (Tags::controller))
        {
            const ControllerDevice device (child);
            controllerDeviceRemoved (device);
        }

        // controller device control removed
        if (parent.getParent().getParent() == objectData && 
            parent.getParent().hasType (Tags::controllers) && 
            parent.hasType (Tags::controller) && 
            child.hasType (Tags::control))
        {
            const ControllerDevice::Control control (child);
            controlRemoved (control);
        }
        
        notifyChanged();
    }

    void Session::valueTreeChildOrderChanged (ValueTree& parent, int, int) {  }
    void Session::valueTreeParentChanged (ValueTree& tree) { }
    void Session::valueTreeRedirected (ValueTree& tree) { }
    
    void Session::saveGraphState()
    {
        for (int i = 0; i < getNumGraphs(); ++i)
            getGraph(i).savePluginState();
    }

    void Session::restoreGraphState()
    {
        for (int i = 0; i < getNumGraphs(); ++i)
            getGraph(i).restorePluginState();
    }
}
