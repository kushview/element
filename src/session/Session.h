/*
    Session.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/ControllerDevice.h"
#include "session/Node.h"

#define EL_TEMPO_MIN 20
#define EL_TEMPO_MAX 999

namespace Element {
    class Session;
    class Globals;
    
    /** Session, the main interface between the engine and model layers */
    class Session : public ObjectModel,
                    public ReferenceCountedObject,
                    public ChangeBroadcaster,
                    public ValueTree::Listener
    {
    public:
        struct ScopedFrozenLock
        {
            ScopedFrozenLock (const Session& s) : session(s)
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
        
        inline int getNumGraphs() const { return objectData.getChildWithName(Tags::graphs).getNumChildren(); }
        inline Node getGraph (const int index) const { return Node (getGraphValueTree(index), false); }
        Node getCurrentGraph() const { return getActiveGraph();}
        Node getActiveGraph() const;
        int getActiveGraphIndex() const;
        
        bool addGraph (const Node &node, const bool setActive);
        
        ValueTree getValueTree() const { return objectData; }
        bool loadData (const ValueTree& data);
        void clear();
        
        inline void setName (const String& name) { setProperty (Slugs::name, name); }
        inline String getName()            const { return objectData.getProperty(Slugs::name, "Invalid Session"); }
        inline Value getNameValue()              { return getPropertyAsValue (Slugs::name); }
        
        inline bool useExternalClock()      const { return (bool) getProperty ("externalSync", false); }
        
        inline bool notificationsFrozen()   const { return freezeChangeNotification; }

        XmlElement* createXml();
        
        void saveGraphState();
        void restoreGraphState();
        
        inline int getNumControllerDevices() const { return getControllerDevicesValueTree().getNumChildren(); }
        inline ValueTree getControllerDeviceValueTree (const int i) const { return getControllerDevicesValueTree().getChild(i); }
        inline ControllerDevice getControllerDevice (const int index) const 
        {
            ControllerDevice device (getControllerDeviceValueTree (index));
            return device;
        }
        
    protected:
        Session();
        friend class Globals;
 
        /** Set a property.  This is a wrapper around the internal ValueTree */
        inline void setProperty (const Identifier& prop, const var& val) { node().setProperty (prop, val, nullptr); }

        friend class ValueTree;
        virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property);
        virtual void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded);
        virtual void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int);
        virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int, int);
        virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged);
        virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged);

    private:
        class Private;
        ScopedPointer<Private> priv;
        void setMissingProperties (bool resetExisting = false);
        
        inline ValueTree getGraphsValueTree() const { return objectData.getChildWithName (Tags::graphs); }
        inline ValueTree getGraphValueTree (const int index) const { return getGraphsValueTree().getChild(index); }
        inline ValueTree getControllerDevicesValueTree() const { return objectData.getChildWithName(Tags::controllers); }
        
        friend class SessionController;
        friend struct ScopedFrozenLock;
        mutable bool freezeChangeNotification = false;
        void notifyChanged();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Session);
    };
    
    typedef ReferenceCountedObjectPtr<Session> SessionPtr;
    typedef SessionPtr SessionRef;
}
