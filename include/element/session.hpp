// Copyright 2023 Kushview, LLC <info@kushview.net>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <element/model.hpp>
#include <element/controller.hpp>
#include <element/node.hpp>
#include <element/signals.hpp>

#define EL_TEMPO_MIN 20
#define EL_TEMPO_MAX 999

#define EL_SESSION_VERSION 1

namespace element {

class Context;

/** Session, the main interface between the model and engine layers */
class Session : public Model,
                public juce::ReferenceCountedObject,
                public juce::ChangeBroadcaster,
                public juce::ValueTree::Listener {
public:
    struct ScopedFrozenLock {
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

    inline int getNumGraphs() const { return objectData.getChildWithName (tags::graphs).getNumChildren(); }
    inline Node getGraph (const int index) const { return Node (getGraphValueTree (index), false); }
    Node getCurrentGraph() const { return getActiveGraph(); }
    Node getActiveGraph() const;
    int getActiveGraphIndex() const;

    bool addGraph (const Node& node, const bool setActive);

    juce::ValueTree getValueTree() const { return objectData; }
    bool loadData (const juce::ValueTree& data);
    void clear();

    inline void setName (const juce::String& name) { setProperty (tags::name, name); }
    inline juce::String getName() const { return objectData.getProperty (tags::name, "Invalid Session"); }
    inline juce::Value getNameValue() { return getPropertyAsValue (tags::name); }

    inline bool useExternalClock() const { return (bool) getProperty ("externalSync", false); }

    inline bool notificationsFrozen() const { return freezeChangeNotification; }

    std::unique_ptr<juce::XmlElement> createXml() const;

    void saveGraphState();
    void restoreGraphState();

    inline int getNumControllers() const { return getControllersValueTree().getNumChildren(); }

    inline juce::ValueTree getControllerValueTree (const int i) const
    {
        return getControllersValueTree().getChild (i);
    }

    inline Controller getController (const int index) const
    {
        Controller device (getControllerValueTree (index));
        return device;
    }

    inline int indexOf (const Controller& device) const
    {
        return getControllersValueTree().indexOf (device.data());
    }

    inline int getNumControllerMaps() const { return getControllerMapsValueTree().getNumChildren(); }
    inline ControllerMap getControllerMap (const int index) const { return ControllerMap (getControllerMapsValueTree().getChild (index)); }
    inline int indexOf (const ControllerMap& controllerMap) const { return getControllerMapsValueTree().indexOf (controllerMap.data()); }

    Node findNodeById (const juce::Uuid&);
    Controller findControllerById (const juce::Uuid&);

    void cleanOrphanControllerMaps();

    typedef std::function<void (const juce::ValueTree& tree)> ValueTreeFunction;
    void forEach (ValueTreeFunction handler) const;

    void setActiveGraph (int index);
    bool containsGraph (const Node& graph) const;

    /** Writes an encoded file */
    bool writeToFile (const juce::File&) const;
    static juce::ValueTree readFromFile (const juce::File&);

    juce::Value getActiveGraphIndexObject (bool syncUpdate = false) const
    {
        return getGraphsValueTree().getPropertyAsValue (tags::active, nullptr, syncUpdate);
    }

    static juce::ValueTree migrate (const juce::ValueTree&, juce::String& error);

protected:
    void forEach (const juce::ValueTree tree, ValueTreeFunction handler) const;

    Session();
    friend class Context;

    /** Set a property. */
    inline void setProperty (const juce::Identifier& prop, const juce::var& val) { objectData.setProperty (prop, val, nullptr); }

    friend class juce::ValueTree;
    virtual void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property);
    virtual void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded);
    virtual void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int);
    virtual void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int, int);
    virtual void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged);
    virtual void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
    void setMissingProperties (bool resetExisting = false);

    inline juce::ValueTree getGraphsValueTree() const { return objectData.getChildWithName (tags::graphs); }
    inline juce::ValueTree getGraphValueTree (const int index) const { return getGraphsValueTree().getChild (index); }
    inline juce::ValueTree getControllersValueTree() const { return objectData.getChildWithName (tags::controllers); }
    inline juce::ValueTree getControllerMapsValueTree() const { return objectData.getChildWithName (tags::maps); }

    friend class SessionService;
    friend class SessionImportWizard;
    friend struct ScopedFrozenLock;
    mutable bool freezeChangeNotification = false;
    void notifyChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Session);

public:
    Signal<void (const Controller&)> controllerDeviceAdded;
    Signal<void (const Controller&)> controllerDeviceRemoved;
    Signal<void (const Control&)> controlAdded;
    Signal<void (const Control&)> controlRemoved;
};

typedef juce::ReferenceCountedObjectPtr<Session> SessionPtr;
typedef SessionPtr SessionRef;

struct ControllerMapObjects {
    ControllerMapObjects() = default;
    ControllerMapObjects (SessionPtr s, const ControllerMap& m)
        : session (s), controllerMap (m)
    {
        if (session != nullptr) {
            device = session->findControllerById (juce::Uuid (controllerMap.getProperty (tags::controller)));
            control = device.findControlById (juce::Uuid (controllerMap.getProperty (tags::control)));
            node = session->findNodeById (juce::Uuid (controllerMap.getProperty (tags::node)));
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
    Controller device;
    Control control;
};
} // namespace element
