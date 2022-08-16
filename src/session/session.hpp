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
#include "session/controllerdevice.hpp"
#include "session/node.hpp"
#include "signals.hpp"

#define EL_TEMPO_MIN 20
#define EL_TEMPO_MAX 999

namespace element {
class Session;
class Context;

/** Session, the main interface between the engine and model layers */
class Session : public ObjectModel,
                public ReferenceCountedObject,
                public ChangeBroadcaster,
                public ValueTree::Listener
{
public:
    struct ScopedFrozenLock
    {
        ScopedFrozenLock (const Session& s) : session (s)
        {
            wasFrozen = session.freezeChangeNotification;
            session.freezeChangeNotification = true;
        }

        ~ScopedFrozenLock()
        {
            session.freezeChangeNotification = wasFrozen;
        }

    private:
        const Session& session;
        bool wasFrozen;
    };

    virtual ~Session();

    inline int getNumGraphs() const { return objectData.getChildWithName (Tags::graphs).getNumChildren(); }
    inline Node getGraph (const int index) const { return Node (getGraphValueTree (index), false); }
    Node getCurrentGraph() const { return getActiveGraph(); }
    Node getActiveGraph() const;
    int getActiveGraphIndex() const;

    bool addGraph (const Node& node, const bool setActive);

    ValueTree getValueTree() const { return objectData; }
    bool loadData (const ValueTree& data);
    void clear();

    inline void setName (const String& name) { setProperty (Tags::name, name); }
    inline String getName() const { return objectData.getProperty (Tags::name, "Invalid Session"); }
    inline Value getNameValue() { return getPropertyAsValue (Tags::name); }

    inline bool useExternalClock() const { return (bool) getProperty ("externalSync", false); }

    inline bool notificationsFrozen() const { return freezeChangeNotification; }

    std::unique_ptr<XmlElement> createXml() const;

    void saveGraphState();
    void restoreGraphState();

    inline int getNumControllerDevices() const { return getControllerDevicesValueTree().getNumChildren(); }

    inline ValueTree getControllerDeviceValueTree (const int i) const
    {
        return getControllerDevicesValueTree().getChild (i);
    }

    inline ControllerDevice getControllerDevice (const int index) const
    {
        ControllerDevice device (getControllerDeviceValueTree (index));
        return device;
    }

    inline int indexOf (const ControllerDevice& device) const
    {
        return getControllerDevicesValueTree().indexOf (device.getValueTree());
    }

    inline int getNumControllerMaps() const { return getControllerMapsValueTree().getNumChildren(); }
    inline ControllerMap getControllerMap (const int index) const { return ControllerMap (getControllerMapsValueTree().getChild (index)); }
    inline int indexOf (const ControllerMap& controllerMap) const { return getControllerMapsValueTree().indexOf (controllerMap.getValueTree()); }

    Node findNodeById (const Uuid&);
    ControllerDevice findControllerDeviceById (const Uuid&);

    void cleanOrphanControllerMaps();

    typedef std::function<void (const ValueTree& tree)> ValueTreeFunction;
    void forEach (ValueTreeFunction handler) const;

    void setActiveGraph (int index);
    bool containsGraph (const Node& graph) const;

    /** Writes an encoded file */
    bool writeToFile (const File&) const;
    static ValueTree readFromFile (const File&);

    Value getActiveGraphIndexObject (bool syncUpdate = false) const
    {
        return getGraphsValueTree().getPropertyAsValue (Tags::active, nullptr, syncUpdate);
    }

protected:
    void forEach (const ValueTree tree, ValueTreeFunction handler) const;

    Session();
    friend class Context;

    /** Set a property. */
    inline void setProperty (const Identifier& prop, const var& val) { objectData.setProperty (prop, val, nullptr); }

    friend class ValueTree;
    virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property);
    virtual void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded);
    virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int);
    virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int, int);
    virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged);
    virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
    void setMissingProperties (bool resetExisting = false);

    inline ValueTree getGraphsValueTree() const { return objectData.getChildWithName (Tags::graphs); }
    inline ValueTree getGraphValueTree (const int index) const { return getGraphsValueTree().getChild (index); }
    inline ValueTree getControllerDevicesValueTree() const { return objectData.getChildWithName (Tags::controllers); }
    inline ValueTree getControllerMapsValueTree() const { return objectData.getChildWithName (Tags::maps); }

    friend class SessionService;
    friend class SessionImportWizard;
    friend struct ScopedFrozenLock;
    mutable bool freezeChangeNotification = false;
    void notifyChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Session);

public:
    Signal<void (const ControllerDevice&)> controllerDeviceAdded;
    Signal<void (const ControllerDevice&)> controllerDeviceRemoved;
    Signal<void (const ControllerDevice::Control&)> controlAdded;
    Signal<void (const ControllerDevice::Control&)> controlRemoved;
};

typedef ReferenceCountedObjectPtr<Session> SessionPtr;
typedef SessionPtr SessionRef;

struct ControllerMapObjects
{
    ControllerMapObjects() = default;
    ControllerMapObjects (SessionPtr s, const ControllerMap& m)
        : session (s), controllerMap (m)
    {
        if (session != nullptr)
        {
            device = session->findControllerDeviceById (Uuid (controllerMap.getProperty (Tags::controller)));
            control = device.findControlById (Uuid (controllerMap.getProperty (Tags::control)));
            node = session->findNodeById (Uuid (controllerMap.getProperty (Tags::node)));
        }
    }

    ControllerMapObjects (const ControllerMapObjects& o) { operator= (o); }
    ControllerMapObjects& operator= (const ControllerMapObjects& o)
    {
        this->session = o.session;
        this->controllerMap = o.controllerMap;
        this->node = o.node;
        this->device = o.device;
        this->control = o.control;
        return *this;
    }

    ~ControllerMapObjects() = default;

    inline bool isValid() const { return device.isValid() && control.isValid() && node.isValid(); }

    SessionPtr session;
    ControllerMap controllerMap;
    Node node;
    ControllerDevice device;
    ControllerDevice::Control control;
};
} // namespace element
