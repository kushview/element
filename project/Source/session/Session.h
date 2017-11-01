/*
    Session.h - This file is part of Element
    Copyright (C) 2016 Kushview, LLC.  All rights reserved.
*/

#pragma once

#include "ElementApp.h"
#include "session/Node.h"

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
        
        inline String getName() const   { return objectData.getProperty(Slugs::name, "Invalid Session"); }
        inline Value getNameValue()     { return getPropertyAsValue (Slugs::name); }
        inline void setName (const String& name) { setProperty (Slugs::name, name); }

        inline bool useExternalClock() const { return (bool) getProperty ("externalSync", false); }
        
        XmlElement* createXml();
        void saveGraphState();
        
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
        
        friend class SessionController;
        bool freezeChangeNotification = false;
        void notifyChanged();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Session);
    };
    
    typedef ReferenceCountedObjectPtr<Session> SessionPtr;
    typedef SessionPtr SessionRef;
}
